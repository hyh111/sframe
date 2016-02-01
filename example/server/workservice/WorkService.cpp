
#include "serv/ServiceDispatcher.h"
#include "WorkService.h"
#include "util/Log.h"

using namespace sframe;

#define MAKE_KEY(_gatesid, _sessionid) (((uint64_t)(_gatesid) << 32) | ((uint64_t)(_sessionid) & 0x00000000ffffffff))

// 初始化（创建服务成功后调用，此时还未开始运行）
void WorkService::Init()
{
	RegistServiceMessageHandler(kWorkMsg_ClientData, &WorkService::OnMsg_ClientData, this);
	RegistServiceMessageHandler(kWorkMsg_EnterWorkService, &WorkService::OnMsg_EnterWorkService, this);
	RegistServiceMessageHandler(kWorkMsg_QuitWorkService, &WorkService::OnMsg_QuitWorkService, this);
}

// 服务接入
void WorkService::OnServiceJoin(const std::unordered_set<int32_t> & sid_set, bool is_remote)
{
	int32_t my_sid = GetServiceId();
	for (int32_t sid : sid_set)
	{
		assert(sid >= kSID_GateServiceBegin && sid <= kSID_GateServiceEnd);
		ServiceDispatcher::Instance().SendServiceMsg(my_sid, sid, (uint16_t)kGateMsg_RegistWorkService, my_sid);
	}
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

	LOG_INFO << "user session(" << gate_sid << ", " << session_id << ") enter work service(" << GetServiceId() << ")" << ENDL;

	if (gate_sid != GetLastServiceId())
	{
		LOG_WARN << "gate service id not equal to last service id" << ENDL;
	}

	std::shared_ptr<User> user = std::make_shared<User>(GetServiceId(), gate_sid, session_id);
	_users.insert(std::make_pair(key, user));
}

void WorkService::OnMsg_QuitWorkService(int32_t gate_sid, int32_t session_id)
{
	uint64_t key = MAKE_KEY(gate_sid, session_id);
	if (_users.erase(key) > 0)
	{
		LOG_INFO << "user session(" << session_id << ") quit work service(" << GetServiceId() << ")" << ENDL;
	}
}