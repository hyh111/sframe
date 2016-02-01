
#include "ServiceDispatcher.h"
#include "ProxyService.h"
#include "../util/Log.h"
#include "../util/TimeHelper.h"

using namespace sframe;

ProxyService::ProxyService() 
	: _listener(nullptr), _session_num(0), _listening(false),
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
	if (_listener != nullptr)
	{
		delete _listener;
	}

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
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SendToRemoteService, &ProxyService::OnMsg_SendToRemoteService, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_AddNewSession, &ProxyService::OnMsg_AddNewSession, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionClosed, &ProxyService::OnMsg_SessionClosed, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionRecvData, &ProxyService::OnMsg_SessionRecvData, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionConnectCompleted, &ProxyService::OnMsg_SessionConnectCompleted, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_ServiceListenerClosed, &ProxyService::OnMsg_ServiceListenerClosed, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionConnectCompleted, &ProxyService::OnMsg_SessionConnectCompleted, this);
}

void ProxyService::OnStart()
{
	// 开启监听器
	if (_listener != nullptr && _listener->Start())
	{
		_listening = true;
	}
}

void ProxyService::OnDestroy()
{
	if (_listener != nullptr)
	{
		_listener->Stop();
	}

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
	return (!_listening && _session_num <= 0);
}

void ProxyService::SetListenAddr(const std::string & ipv4, uint16_t port, const std::string & key)
{
	assert(!ipv4.empty());

	if (_listener != nullptr)
	{
		return;
	}

	_listener = new ServiceListener();
	_listener->SetListenAddr(ipv4, port);
	_local_key = key;
}

// 处理周期定时器
void ProxyService::OnCycleTimer()
{
	int64_t cur_time = sframe::TimeHelper::GetEpochMilliseconds();
	_timer_mgr.Execute(cur_time);
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
		for (auto it = it_info->second.msg_cache.begin(); it < it_info->second.msg_cache.end(); it++)
		{
			_session[session_id]->SendData(&(*(*it))[0], (int32_t)(*it)->size());
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


void ProxyService::OnMsg_SendToRemoteService(int32_t sid, std::shared_ptr<std::vector<char>> & data)
{
	auto it = _sid_to_sessioninfo.find(sid);
	if (it == _sid_to_sessioninfo.end())
	{
		return;
	}

	if (_session[it->second.session_id] == nullptr || _session[it->second.session_id]->GetState() != ServiceSession::kSessionState_Running)
	{
		// 将消息缓存
		it->second.msg_cache.push_back(data);
	}
	else
	{
		_session[it->second.session_id]->SendData(&(*data)[0], (int32_t)data->size());
	}
}

void ProxyService::OnMsg_AddNewSession(std::shared_ptr<sframe::TcpSocket> & sock)
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

void ProxyService::OnMsg_ServiceListenerClosed()
{
	_listening = false;
}