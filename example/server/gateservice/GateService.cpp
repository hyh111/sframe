
#include <assert.h>
#include "serv/ServiceDispatcher.h"
#include "GateService.h"
#include "util/Log.h"
#include "util/RandomHelper.h"

// 初始化（创建服务成功后调用，此时还未开始运行）
void GateService::Init()
{
	RegistInsideServiceMessageHandler(kGateMsg_SessionClosed, &GateService::OnMsg_SessionClosed, this);
	RegistInsideServiceMessageHandler(kGateMsg_SessionRecvData, &GateService::OnMsg_SessionRecvData, this);

	RegistServiceMessageHandler(kGateMsg_RegistWorkService, &GateService::OnMsg_RegistWorkService, this);
	RegistServiceMessageHandler(kGateMsg_SendToClient, &GateService::OnMsg_SendToClient, this);
}

// 新连接到来
void GateService::OnNewConnection(const sframe::ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock)
{
	int32_t work_service = ChooseWorkService();
	if (work_service <= 0)
	{
		sock->Close();
		return;
	}

	assert(work_service <= sframe::ServiceDispatcher::kMaxServiceId);

	int32_t sessionid = _new_session_id++;
	std::shared_ptr<ClientSession> session = std::make_shared<ClientSession>(this, sessionid, sock);
	session->EnterWorkService(work_service);
	_sessions.insert(std::make_pair(sessionid, session));
	FLOG(GetLogName()) << "NewSession|" << sessionid << "|WorkService|" << work_service << ENDL;
}

// 服务断开（仅与本服务发生过消息往来的服务断开时，才会有通知）
void GateService::OnServiceLost(const std::vector<int32_t> & services)
{
	for (int32_t lost_sid : services)
	{
		if (_usable_work_service.erase(lost_sid) > 0)
		{
			FLOG(GetLogName()) << "Lost WorkService|" << lost_sid << std::endl;
		}
	}
}

int32_t GateService::ChooseWorkService()
{
	if (_usable_work_service.empty())
	{
		return -1;
	}

	int32_t index = sframe::Rand(0, (int)_usable_work_service.size());
	for (int32_t sid : _usable_work_service)
	{
		if (index == 0)
		{
			return sid;
		}

		index--;
	}

	return -1;
}

const std::string & GateService::GetLogName()
{
	if (_log_name.empty())
	{
		_log_name = "GateService" + std::to_string(GetServiceId());
	}

	return _log_name;
}

void GateService::OnMsg_SessionClosed(const std::shared_ptr<ClientSession> & session)
{
	FLOG(GetLogName()) << "SessionClose|" << session->GetSessionId() << ENDL;
	session->HandleClosed();
	_sessions.erase(session->GetSessionId());
}

void GateService::OnMsg_SessionRecvData(const std::shared_ptr<ClientSession> & session, const std::shared_ptr<std::vector<char>> & data)
{
	assert(session != nullptr && data != nullptr && session->GetWorkService() > 0);
	session->SendToWorkService(data);
}


void GateService::OnMsg_RegistWorkService(int32_t work_sid)
{
	_usable_work_service.insert(work_sid);
	FLOG(GetLogName()) << "Regist WorkService|" << work_sid << std::endl;
}


void GateService::OnMsg_SendToClient(const GateMsg_SendToClient & data)
{
	auto it = _sessions.find(data.session_id);
	if (it != _sessions.end())
	{
		it->second->SendToClient(data.client_data);
	}
}