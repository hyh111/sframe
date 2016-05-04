
#include "ServiceDispatcher.h"
#include "ProxyService.h"
#include "../util/Log.h"
#include "../util/TimeHelper.h"

using namespace sframe;

ProxyService::ProxyService() 
	:  _session_num(0), _listening(false),
	_timer_mgr(std::bind(&ProxyService::GetServiceSessionById, this, std::placeholders::_1))
{
	memset(_session, 0, sizeof(_session));

	// 初始化session_id队列
	for (int i = 1; i <= kMaxSessionNumber; i++)
	{
		_session_id_queue.push(i);
	}
}

ProxyService::~ProxyService()
{
	for (int i = 1; i <= kMaxSessionNumber; i++)
	{
		if (_session[i] != nullptr)
		{
			delete _session[i];
		}
	}
}

void ProxyService::Init()
{
	// 注册消息处理函数
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionClosed, &ProxyService::OnMsg_SessionClosed, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionRecvData, &ProxyService::OnMsg_SessionRecvData, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionConnectCompleted, &ProxyService::OnMsg_SessionConnectCompleted, this);
}

void ProxyService::OnDestroy()
{
	for (int i = 1; i <= kMaxSessionNumber; i++)
	{
		if (_session[i] != nullptr)
		{
			_session[i]->Close();
		}
	}
}

bool ProxyService::IsDestroyCompleted() const
{
	return (_session_num <= 0);
}

// 处理周期定时器
void ProxyService::OnCycleTimer()
{
	int64_t cur_time = sframe::TimeHelper::GetEpochMilliseconds();
	_timer_mgr.Execute(cur_time);
}

// 新连接到来
void ProxyService::OnNewConnection(const std::shared_ptr<sframe::TcpSocket> & sock)
{
	int32_t session_id = GetNewSessionId();
	if (session_id <= 0)
	{
		sock->Close();
		LOG_INFO << "ServiceSession number upper limit, current session number: " << _session_num << ENDL;
		return;
	}

	assert(_session[session_id] == nullptr);
	_session[session_id] = new ServiceSession(session_id, this, sock);
	_session_num++;
	_session[session_id]->Init();
}

// 代理服务消息
void ProxyService::OnProxyServiceMessage(const std::shared_ptr<ProxyServiceMessage> & msg)
{
	auto it = _sid_to_sessioninfo.find(msg->dest_sid);
	if (it == _sid_to_sessioninfo.end())
	{
		return;
	}

	auto service_session = _session[it->second.session_id];

	if (service_session == nullptr || service_session->GetState() != ServiceSession::kSessionState_Running)
	{
		// 将消息缓存
		it->second.msg_cache.push_back(msg);
	}
	else
	{
		char data[65536];
		int32_t len = 65536;
		if (msg->Serialize(data, &len))
		{
			service_session->SendData(data, len);
		}
	}
}

// 注册会话
// 返回会话ID，小于0失败
int32_t ProxyService::RegistSession(const std::string & remote_ip, uint16_t remote_port, const std::string & remote_key)
{
	int32_t session_id = GetNewSessionId();
	if (session_id <= 0)
	{
		return -1;
	}

	assert(_session[session_id] == nullptr);
	_session[session_id] = new ServiceSession(session_id, this, remote_ip, remote_port, remote_key);
	_session_num++;
	_session[session_id]->Init();

	return session_id;
}

// 注册会话定时器
int32_t ProxyService::RegistSessionTimer(int32_t session_id, int32_t after_ms, sframe::ObjectTimerManager<int32_t, ServiceSession>::TimerFunc func)
{
	assert(after_ms >= 0 && session_id > 0 && session_id <= kMaxSessionNumber && _session[session_id]);
	int64_t cur_time = sframe::TimeHelper::GetEpochMilliseconds();
	return _timer_mgr.Regist(cur_time + after_ms, session_id, func);
}

// 开始会话
void ProxyService::StartSession(int32_t session_id, const std::vector<int32_t> & remote_sid)
{
	assert(_session[session_id]);

	std::unordered_set<int32_t> new_join;

	// 添加sid到sessionid的关联，并检查是否有消息缓存
	for (int32_t sid : remote_sid)
	{
		auto it_info = _sid_to_sessioninfo.find(sid);
		if (it_info == _sid_to_sessioninfo.end())
		{
			new_join.insert(sid);
			auto insert_ret = _sid_to_sessioninfo.insert(std::make_pair(sid, SessionInfo()));
			assert(insert_ret.second);
			it_info = insert_ret.first;
		}

		if (it_info->second.session_id != 0)
		{
			LOG_INFO << "remote service(" << sid << ") already attached to service session(" << it_info->second.session_id << "), ignored it" << ENDL;
			continue;
		}

		it_info->second.session_id = session_id;
		// 发送所有已缓存的消息
		char send_data[65536];
		for (auto it = it_info->second.msg_cache.begin(); it < it_info->second.msg_cache.end(); it++)
		{
			int32_t data_len = sizeof(send_data);
			if ((*it)->Serialize(send_data, &data_len))
			{
				_session[session_id]->SendData(send_data, data_len);
			}
		}
	}

	ServiceDispatcher::Instance().NotifyServiceJoin(new_join, true);
}

int32_t ProxyService::GetNewSessionId()
{
	if (_session_id_queue.empty())
	{
		return -1;
	}

	int32_t session_id = _session_id_queue.front();
	_session_id_queue.pop();
	return session_id;
}

ServiceSession * ProxyService::GetServiceSessionById(int32_t session_id)
{
	assert(session_id > 0 && session_id <= kMaxSessionNumber);
	return _session[session_id];
}


void ProxyService::OnMsg_SessionClosed(int32_t session_id)
{
	assert(_session[session_id]);

	// 清除sid和session的关联
	for (auto it = _sid_to_sessioninfo.begin(); it != _sid_to_sessioninfo.end(); it++)
	{
		if (it->second.session_id == session_id)
		{
			it->second.session_id = 0;
		}
	}

	if (!_session[session_id]->TryFree())
	{
		return;
	}

	delete _session[session_id];
	_session[session_id] = nullptr;
	_session_id_queue.push(session_id);
	_session_num--;
}

void ProxyService::OnMsg_SessionRecvData(int32_t session_id, const std::shared_ptr<std::vector<char>> & data)
{
	assert(_session[session_id]);
	_session[session_id]->DoRecvData(*(data.get()));
}

void ProxyService::OnMsg_SessionConnectCompleted(int32_t session_id, bool success)
{
	assert(_session[session_id]);
	_session[session_id]->DoConnectCompleted(success);
}
