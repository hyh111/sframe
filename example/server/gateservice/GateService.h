
#ifndef __GATE_SERVICE_H__
#define __GATE_SERVICE_H__

#include <unordered_map>
#include <unordered_set>
#include "ClientSession.h"
#include "serv/Service.h"
#include "../config/ServiceDef.h"
#include "../ssproto/SSMsg.h"

class GateService : public sframe::Service
{
public:
	GateService() : _new_session_id(1) {}
	virtual ~GateService(){}

	// 初始化（创建服务成功后调用，此时还未开始运行）
	void Init() override;

	// 新连接到来
	void OnNewConnection(const std::shared_ptr<sframe::TcpSocket> & sock) override;

	// 是否销毁完成
	bool IsDestroyCompleted() const override
	{
		return _sessions.empty();
	}

private:
	int32_t ChooseWorkService();

private:
	void OnMsg_SessionClosed(const std::shared_ptr<ClientSession> & session);
	void OnMsg_SessionRecvData(const std::shared_ptr<ClientSession> & session, const std::shared_ptr<std::vector<char>> & data);

	void OnMsg_RegistWorkService(int32_t work_sid);
	void OnMsg_SendToClient(const GateMsg_SendToClient & data);

private:
	int32_t _new_session_id;
	std::unordered_map<int32_t, std::shared_ptr<ClientSession>> _sessions;
	std::unordered_set<int32_t> _usable_work_service;
};

#endif
