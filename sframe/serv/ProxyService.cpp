
#include "ServiceDispatcher.h"
#include "ProxyService.h"
#include "../net/SocketAddr.h"
#include "../util/Log.h"
#include "../util/TimeHelper.h"

using namespace sframe;


ProxyService::ProxyService() : _have_no_session(true), _listening(false), _cur_max_session_id(0), _session_id_first_loop(true)
{
	memset(_quick_find_session_arr, 0, sizeof(_quick_find_session_arr));
}

ProxyService::~ProxyService()
{
	for (auto & pr : _all_sessions)
	{
		if (pr.second)
		{
			delete pr.second;
		}
	}
}

void ProxyService::Init()
{
	// 注册消息处理函数
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionClosed, &ProxyService::OnMsg_SessionClosed, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionRecvData, &ProxyService::OnMsg_SessionRecvData, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SessionConnectCompleted, &ProxyService::OnMsg_SessionConnectCompleted, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_AdminCommand, &ProxyService::OnMsg_AdminCommand, this);
	this->RegistInsideServiceMessageHandler(kProxyServiceMsgId_SendAdminCommandResponse, &ProxyService::OnMsg_SendAdminCommandResponse, this);
}

void ProxyService::OnDestroy()
{
	for (auto & pr : _all_sessions)
	{
		if (pr.second)
		{
			pr.second->Close();
		}
	}
}

bool ProxyService::IsDestroyCompleted() const
{
	return _have_no_session;
}

// 处理周期定时器
void ProxyService::OnCycleTimer()
{
	_timer_mgr.Execute();
}

// 新连接到来
void ProxyService::OnNewConnection(const ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock)
{
	static const std::string admin_addr_desc = "AdminAddr";

	Error err = sock->SetTcpNodelay(true);
	if (err)
	{
		LOG_WARN << "Set tcp nodelay error|" << err.Code() << "|" << sframe::ErrorMessage(err).Message() << ENDL;
	}

	int32_t session_id = GetNewSessionId();
	if (session_id < 0)
	{
		sock->Close();
		LOG_ERROR << "ServiceSession number upper limit|session number|" << _all_sessions.size() << ENDL;
		return;
	}

	if (listen_addr_info.desc_name == admin_addr_desc)
	{
		AddServiceSession(session_id, new AdminSession(session_id, this, sock));
	}
	else
	{
		AddServiceSession(session_id, new ServiceSession(session_id, this, sock));
	}
}

// 代理服务消息
void ProxyService::OnProxyServiceMessage(const std::shared_ptr<ProxyServiceMessage> & msg)
{
	auto it = _sid_to_sessionid.find(msg->dest_sid);
	if (it == _sid_to_sessionid.end())
	{
		return;
	}

	int32_t sessionid = it->second;
	ServiceSession * session = GetServiceSession(sessionid);
	if (session)
	{
		// 调用发送
		session->SendData(msg);
	}
	else
	{
		assert(false);
	}
}

#define MAKE_ADDR_INFO(ip, port) ((((int64_t)(ip) & 0xffffffff) << 16) | ((int64_t)(port) & 0xffff))

// 注册会话
// 返回会话ID，小于0失败
int32_t ProxyService::RegistSession(int32_t sid, const std::string & remote_ip, uint16_t remote_port)
{
	auto it_sid_to_sessionid = _sid_to_sessionid.find(sid);
	if (it_sid_to_sessionid != _sid_to_sessionid.end())
	{
		assert(it_sid_to_sessionid->second > 0);
		return it_sid_to_sessionid->second;
	}

	int32_t session_id = -1;
	ServiceSession * session = nullptr;
	SocketAddr sock_addr(remote_ip.c_str(), remote_port);
	int64_t addr_info = MAKE_ADDR_INFO(sock_addr.GetIp(), sock_addr.GetPort());
	auto it_session_id = _session_addr_to_sessionid.find(addr_info);
	if (it_session_id == _session_addr_to_sessionid.end())
	{
		// 若没有相同目的地址的session，新建一个
		session_id = GetNewSessionId();
		if (session_id < 0)
		{
			return -1;
		}

		session = new ServiceSession(session_id, this, remote_ip, remote_port);
		AddServiceSession(session_id, session);
		_session_addr_to_sessionid[addr_info] = session_id;
	}
	else
	{
		session_id = it_session_id->second;
		session = GetServiceSession(session_id);
	}

	assert(session && session_id == session->GetSessionId());
	// 添加会话包含的服务
	_sessionid_to_sid[session_id].insert(sid);
	// 添加sid到sessionid的映射
	_sid_to_sessionid[sid] = session_id;

	return session_id;
}

// 注册管理命令处理处理方法
void ProxyService::RegistAdminCmd(const std::string & cmd, const AdminCmdHandleFunc & func)
{
	_map_admin_cmd_func[cmd] = func;
}


static const int32_t kMaxSessionId = 2000000000;

int32_t ProxyService::GetNewSessionId()
{
	assert(_cur_max_session_id < kMaxSessionId);

	int32_t session_id = -1;

	if (_session_id_first_loop)
	{
		session_id = (++_cur_max_session_id);
		if (_cur_max_session_id >= kMaxSessionId)
		{
			_cur_max_session_id = 0;
			_session_id_first_loop = false;
		}
	}
	else
	{
		int32_t old_max_session_id = _cur_max_session_id;
		while (true)
		{
			session_id = (++_cur_max_session_id);
			if (_cur_max_session_id >= kMaxSessionId)
			{
				_cur_max_session_id = 0;
			}

			if (_all_sessions.find(session_id) == _all_sessions.end())
			{
				break;
			}
			if (_cur_max_session_id == old_max_session_id)
			{
				session_id = -1;
				break;
			}
		}
	}

	return session_id;
}

ServiceSession * ProxyService::GetServiceSession(int32_t session_id)
{
	if (session_id >= 0 && session_id < kQuickFindSessionArrLen)
	{
		return _quick_find_session_arr[session_id];
	}

	auto it = _all_sessions.find(session_id);
	return it == _all_sessions.end() ? nullptr : it->second;
}

void ProxyService::AddServiceSession(int32_t session_id, ServiceSession * session)
{
	if (!_all_sessions.insert(std::make_pair(session_id, session)).second)
	{
		assert(false);
		return;
	}

	if (session_id >= 0 && session_id < kQuickFindSessionArrLen)
	{
		assert(_quick_find_session_arr[session_id] == nullptr);
		_quick_find_session_arr[session_id] = session;
	}

	session->SetTimerManager(&_timer_mgr);
	session->Init();
	_have_no_session = false;
}

void ProxyService::DeleteServiceSession(int32_t session_id)
{
	auto it = _all_sessions.find(session_id);
	if (it == _all_sessions.end())
	{
		assert(false);
		return;
	}

	delete it->second;
	_all_sessions.erase(it);

	if (session_id >= 0 && session_id < kQuickFindSessionArrLen)
	{
		assert(_quick_find_session_arr[session_id]);
		_quick_find_session_arr[session_id] = nullptr;
	}

	if (_all_sessions.empty())
	{
		_have_no_session = true;
	}
}

void ProxyService::OnMsg_SessionClosed(bool by_self, int32_t session_id)
{
	ServiceSession * session = GetServiceSession(session_id);
	assert(session && session->GetSessionId() == session_id);

	// 是否要删除session
	if (!session->TryFree())
	{
		return;
	}

	// 删除session
	DeleteServiceSession(session_id);

	// 删除其他记录
	auto it_sid = _sessionid_to_sid.find(session_id);
	if (it_sid != _sessionid_to_sid.end())
	{
		for (int32_t rm_sid : it_sid->second)
		{
			auto it_sid_to_sessionid = _sid_to_sessionid.find(rm_sid);
			if (it_sid_to_sessionid != _sid_to_sessionid.end() && it_sid_to_sessionid->second == session_id)
			{
				_sid_to_sessionid.erase(rm_sid);
			}
			else
			{
				assert(false);
			}
		}

		_sessionid_to_sid.erase(session_id);
	}
}

static const uint16_t kMaxPackSize = 65536 - sizeof(uint16_t);

void ProxyService::OnMsg_SessionRecvData(int32_t session_id, const std::shared_ptr<std::vector<char>> & data)
{
	ServiceSession * session = GetServiceSession(session_id);
	if (session == nullptr)
	{
		assert(false);
		LOG_ERROR << "ServiceSession not existed|" << session_id << std::endl;
		return;
	}

	std::vector<char> & vec_data = *data;
	char * p = &vec_data[0];
	uint32_t len = (uint32_t)vec_data.size();
	StreamReader reader(p, len);

	// 读取消息头部
	int32_t src_sid = 0;
	int32_t dest_sid = 0;
	int64_t msg_session_key = 0;
	uint16_t msg_id = 0;
	if (!AutoDecode(reader, src_sid, dest_sid, msg_session_key, msg_id))
	{
		LOG_ERROR << "Recv from remote server(" << session->GetRemoteAddrText() << "), but decode error" << std::endl;
		return;
	}

	// 源服务ID是否和本地服务ID冲突
	if (ServiceDispatcher::Instance().IsLocalService(src_sid))
	{
		LOG_ERROR << "Recv from remote server(" << session->GetRemoteAddrText()
			<< "), but src_id conflict with local service " << src_sid << std::endl;
		return;
	}

	// 查找远程服务是否已经关联了session，若还没有关联，在这里关联
	// 关联远程服务ID与Session
	if (_sid_to_sessionid.insert(std::make_pair(src_sid, session_id)).second)
	{
		_sessionid_to_sid[session_id].insert(src_sid);
	}

	if (ServiceDispatcher::Instance().IsLocalService(dest_sid))
	{
		// 封装消息并发送到目标本地服务
		int32_t data_len = (int32_t)len - (int32_t)reader.GetReadedLength();
		assert(data_len >= 0);
		std::shared_ptr<NetServiceMessage> msg = std::make_shared<NetServiceMessage>();
		msg->dest_sid = dest_sid;
		msg->src_sid = src_sid;
		msg->session_key = msg_session_key;
		msg->msg_id = msg_id;
		msg->data = std::move(vec_data);
		// 发送到目标服务
		ServiceDispatcher::Instance().SendMsg(dest_sid, msg);
	}
	else
	{
		// 是否有对应的远程服务会话
		auto it = _sid_to_sessionid.find(dest_sid);
		if (it == _sid_to_sessionid.end())
		{
			LOG_ERROR << "Recv from remote server(" << session->GetRemoteAddrText()
				<< "), but can not find dest service " << dest_sid << std::endl;
			return;
		}

		// 转发到远程服务
		ServiceSession * other_session = GetServiceSession(it->second);
		if (other_session)
		{
			uint16_t msg_size = (uint16_t)vec_data.size();
			if (msg_size >= kMaxPackSize)
			{
				// 在发送网络消息处有检测，这里一般不可能发生
				msg_size = kMaxPackSize;
			}
			msg_size = HTON_16(msg_size);
			// 调用发送
			other_session->SendData((const char *)&msg_size, sizeof(uint16_t));
			other_session->SendData(vec_data.data(), msg_size);

			LOG_WARN << "Recv from remote server(" << session->GetRemoteAddrText() << "), but there is no local service " 
				<< dest_sid << ", has been forwarded the message to other server(" << other_session->GetRemoteAddrText() << ')' << std::endl;
		}
		else
		{
			assert(false);
		}
	}
}

void ProxyService::OnMsg_SessionConnectCompleted(int32_t session_id, bool success)
{
	ServiceSession * session = GetServiceSession(session_id);
	if (session)
	{
		session->DoConnectCompleted(success);
	}
	else
	{
		assert(false);
	}
}

void ProxyService::OnMsg_AdminCommand(int32_t admin_session_id, const std::shared_ptr<sframe::HttpRequest> & http_req)
{
	ServiceSession * session = GetServiceSession(admin_session_id);
	if (session == nullptr)
	{
		assert(false);
		LOG_ERROR << "Cannot fand session " << admin_session_id << std::endl;
		return;
	}

	// 只支持GET
	if (http_req->GetMethod() != "GET")
	{
		HttpResponse http_resp;
		http_resp.SetProtoVersion(http_req->GetProtoVersion());
		http_resp.SetStatusCode(405);
		http_resp.SetStatusDesc("Method Not Allowed");
		http_resp.SetHeader("Connection", "Keep-Alive");
		http_resp.SetHeader("Allow", "GET");
		std::string resp_content = http_resp.ToString();
		session->SendData(resp_content.data(), resp_content.length());
		LOG_ERROR << "Recv method not allowed admin cmd from client(" << session->GetRemoteAddrText() << ")|" << http_req->GetMethod() << std::endl;
		return;
	}

	AdminCmd admin_cmd(admin_session_id);
	if (!admin_cmd.Parse(http_req))
	{
		HttpResponse http_resp;
		http_resp.SetProtoVersion(http_req->GetProtoVersion());
		http_resp.SetStatusCode(400);
		http_resp.SetStatusDesc("Bad Request");
		http_resp.SetHeader("Connection", "Keep-Alive");
		std::string resp_content = http_resp.ToString();
		session->SendData(resp_content.data(), resp_content.length());
		LOG_ERROR << "Recv invalid admin cmd from client(" << session->GetRemoteAddrText() << ")|" << http_req->GetMethod() << '|'
			<< http_req->GetRequestUrl() << '|' << http_req->GetRequestParam() << std::endl;
		return;
	}

	auto it = _map_admin_cmd_func.find(admin_cmd.GetCmdName());
	if (it == _map_admin_cmd_func.end())
	{
		HttpResponse http_resp;
		http_resp.SetProtoVersion(http_req->GetProtoVersion());
		http_resp.SetStatusCode(404);
		http_resp.SetStatusDesc("Not Found");
		http_resp.SetHeader("Connection", "Keep-Alive");
		std::string resp_content = http_resp.ToString();
		session->SendData(resp_content.data(), resp_content.length());
		LOG_ERROR << "Recv unknown admin cmd from client(" << session->GetRemoteAddrText() << ")|" << admin_cmd.ToString() << std::endl;
		return;
	}

	LOG_INFO << "Recv admin cmd from client(" << session->GetRemoteAddrText() << ")|" << admin_cmd.ToString() << std::endl;

	// 调用处理函数
	it->second(admin_cmd);
}

void ProxyService::OnMsg_SendAdminCommandResponse(int32_t admin_session_id, const std::string & data, const std::shared_ptr<sframe::HttpRequest> & http_req)
{
	ServiceSession * session = GetServiceSession(admin_session_id);
	if (session)
	{
		HttpResponse http_resp;
		if (http_req)
		{
			http_resp.SetProtoVersion(http_req->GetProtoVersion());
		}
		http_resp.SetStatusCode(200);
		http_resp.SetStatusDesc("OK");
		http_resp.SetHeader("Connection", "Keep-Alive");
		http_resp.SetContent(data); 
		std::string resp_content = http_resp.ToString();
		session->SendData(resp_content.data(), resp_content.length());
	}
}
