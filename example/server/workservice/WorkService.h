
#ifndef __WORK_SERVICE_H__
#define __WORK_SERVICE_H__

#include <unordered_map>
#include "serv/Service.h"
#include "User.h"
#include "util/ObjectFactory.h"

class WorkService : public sframe::Service, public sframe::RegFactoryByName<WorkService>
{
public:
	WorkService() {}
	virtual ~WorkService() {}

	// 初始化（创建服务成功后调用，此时还未开始运行）
	void Init() override;

private:
	void OnMsg_EnterWorkService(int32_t gate_sid, int64_t session_id);

	void OnMsg_QuitWorkService(int32_t gate_sid, int64_t session_id);

	const std::string & GetLogName();

	User * GetUser(int64_t session_id);

private:
	std::unordered_map<int64_t, std::shared_ptr<User>> _users;
	std::string _log_name;
};

#endif
