
#include "serv/ServiceDispatcher.h"
#include "WorkService.h"
#include "util/Log.h"
#include "../config/ServerConfig.h"

using namespace sframe;

#define MAKE_KEY(_gatesid, _sessionid) (((uint64_t)(_gatesid) << 32) | ((uint64_t)(_sessionid) & 0x00000000ffffffff))

// 初始化（创建服务成功后调用，此时还未开始运行）
void WorkService::Init()
{
	RegistServiceMessageHandler(kWorkMsg_ClientData, &WorkService::OnMsg_ClientData, this);
	RegistServiceMessageHandler(kWorkMsg_EnterWorkService, &WorkService::OnMsg_EnterWorkService, this);
	RegistServiceMessageHandler(kWorkMsg_QuitWorkService, &WorkService::OnMsg_QuitWorkService, this);
}


void WorkService::OnMsg_ClientData(const WorkMsg_ClientData & msg)
{
	uint64_t key = MAKE_KEY(msg.gate_sid, msg.session_id);
	auto it_user = _users.find(key);
	if (it_user == _users.end())
	{
		return;
	}

	it_user->second->OnClientMsg(*msg.client_data);
}

void WorkService::OnMsg_EnterWorkService(int32_t gate_sid, int32_t session_id)
{
	uint64_t key = MAKE_KEY(gate_sid, session_id);
	if (_users.find(key) != _users.end())
	{
		return;
	}

	FLOG(GetLogName()) << "User Enter|GateService|" << gate_sid << "|Session ID|" << session_id << std::endl;
	if (gate_sid != GetLastServiceId())
	{
		FLOG(GetLogName()) << "gate service id not equal to last service id" << std::endl;
	}

	std::shared_ptr<User> user = std::make_shared<User>(GetServiceId(), gate_sid, session_id);
	_users.insert(std::make_pair(key, user));
}

void WorkService::OnMsg_QuitWorkService(int32_t gate_sid, int32_t session_id)
{
	uint64_t key = MAKE_KEY(gate_sid, session_id);
	if (_users.erase(key) > 0)
	{
		FLOG(GetLogName()) << "User Quit|GateService|" << gate_sid << "|Session ID|" << session_id << std::endl;
	}
}

const std::string & WorkService::GetLogName()
{
	if (_log_name.empty())
	{
		_log_name = "WorkService" + std::to_string(GetServiceId());
	}

	return _log_name;
}