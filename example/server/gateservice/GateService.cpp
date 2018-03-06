
#include <assert.h>
#include "serv/ServiceDispatcher.h"
#include "GateService.h"
#include "util/Log.h"
#include "util/RandomHelper.h"
#include "../config/ServerConfig.h"

// ³õÊ¼»¯£¨´´½¨·þÎñ³É¹¦ºóµ÷ÓÃ£¬´ËÊ±»¹Î´¿ªÊ¼ÔËÐÐ£©
void GateService::Init()
{
	RegistInsideServiceMessageHandler(kGateMsg_SessionClosed, &GateService::OnMsg_SessionClosed, this);
	RegistInsideServiceMessageHandler(kGateMsg_SessionRecvData, &GateService::OnMsg_SessionRecvData, this);
	RegistServiceMessageHandler(kGateMsg_SendToClient, &GateService::OnMsg_SendToClient, this);

	// »ñÈ¡ÅäÖÃµÄËùÓÐÂß¼­·þÎñ
	auto & gate_services = ServerConfig::Instance().type_to_services["WorkService"];
	for (auto & it_pair : gate_services)
	{
		_work_services.push_back(it_pair.first);
	}
}

// ÐÂÁ¬½Óµ½À´
void GateService::OnNewConnection(const sframe::ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock)
{
	sframe::Error err = sock->SetTcpNodelay(true);
	if (err)
	{
		FLOG(GetLogName()) << "Set tcp nodelay error|" << err.Code() << "|" << sframe::ErrorMessage(err).Message() << ENDL;
	}

	int32_t work_service = ChooseWorkService();
	if (work_service <= 0)
	{
		sock->Close();
		return;
	}

	int64_t cur_sid = GetServiceId();
	int64_t sessionid = _new_session_id++;
	sessionid = sessionid | (cur_sid << 32);
	std::shared_ptr<ClientSession> session = std::make_shared<ClientSession>(this, sessionid, sock);
	session->EnterWorkService(work_service);
	_sessions.insert(std::make_pair(sessionid, session));
	FLOG(GetLogName()) << "NewSession|" << sessionid << "|WorkService|" << work_service << ENDL;
}

// ´¦ÀíÏú»Ù
void GateService::OnDestroy()
{
	for (auto & pr : _sessions)
	{
		pr.second->StartClose();
	}
}

int32_t GateService::ChooseWorkService()
{
	if (_work_services.empty())
	{
		return -1;
	}

	_last_choosed_work_service++;
	if (_last_choosed_work_service >= (int32_t)_work_services.size())
	{
		_last_choosed_work_service = 0;
	}

	return _work_services[_last_choosed_work_service];
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


void GateService::OnMsg_SendToClient(const GateMsg_SendToClient & data)
{
	auto it = _sessions.find(data.session_id);
	if (it != _sessions.end())
	{
		it->second->SendToClient(data.client_data);
	}
}