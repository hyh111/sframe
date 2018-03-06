
#include "serv/ServiceDispatcher.h"
#include "WorkService.h"
#include "util/Log.h"
#include "../config/ServerConfig.h"

using namespace sframe;

#define MAKE_KEY(_gatesid, _sessionid) (((uint64_t)(_gatesid) << 32) | ((uint64_t)(_sessionid) & 0x00000000ffffffff))

// ³õÊ¼»¯£¨´´½¨·þÎñ³É¹¦ºóµ÷ÓÃ£¬´ËÊ±»¹Î´¿ªÊ¼ÔËÐÐ£©
void WorkService::Init()
{
	RegistServiceMessageHandler(kWorkMsg_EnterWorkService, &WorkService::OnMsg_EnterWorkService, this);
	RegistServiceMessageHandler(kWorkMsg_QuitWorkService, &WorkService::OnMsg_QuitWorkService, this);

	std::function<User*(const int64_t &)> get_user_func = std::bind(&WorkService::GetUser, this, std::placeholders::_1);
	RegistServiceMessageHandler(kWorkMsg_ClientData, &User::OnClientData, get_user_func);
}


void WorkService::OnMsg_EnterWorkService(int32_t gate_sid, int64_t session_id)
{
	if (_users.find(session_id) != _users.end())
	{
		return;
	}

	FLOG(GetLogName()) << "User Enter|GateService|" << gate_sid << "|Session ID|" << session_id << std::endl;
	if (gate_sid != GetSenderServiceId())
	{
		FLOG(GetLogName()) << "gate service id not equal to last service id" << std::endl;
	}

	std::shared_ptr<User> user = std::make_shared<User>(GetServiceId(), gate_sid, session_id);
	_users.insert(std::make_pair(session_id, user));
}

void WorkService::OnMsg_QuitWorkService(int32_t gate_sid, int64_t session_id)
{
	if (_users.erase(session_id) > 0)
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

User * WorkService::GetUser(int64_t session_id)
{
	auto it = _users.find(session_id);
	if (it == _users.end())
	{
		return nullptr;
	}

	return it->second.get();
}