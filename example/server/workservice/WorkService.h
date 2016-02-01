
#ifndef __WORK_SERVICE_H__
#define __WORK_SERVICE_H__

#include <unordered_map>
#include "serv/Service.h"
#include "../config/ServiceDef.h"
#include "../ssproto/SSMsg.h"
#include "User.h"

class WorkService : public sframe::Service
{
public:
	WorkService() {}
	virtual ~WorkService() {}

	// 初始化（创建服务成功后调用，此时还未开始运行）
	void Init() override;

	// 服务接入
	void OnServiceJoin(const std::unordered_set<int32_t> & sid_set, bool is_remote) override;

	// 是否关心指定服务的接入
	bool IsCareServiceJoin(int32_t sid) const
	{
		if (sid >= kSID_GateServiceBegin && sid <= kSID_GateServiceEnd)
		{
			return true;
		}
		return false;
	}

private:
	void OnMsg_ClientData(const WorkMsg_ClientData & msg);

	void OnMsg_EnterWorkService(int32_t gate_sid, int32_t session_id);

	void OnMsg_QuitWorkService(int32_t gate_sid, int32_t session_id);

private:
	std::unordered_map<uint64_t, std::shared_ptr<User>> _users;
};

#endif
