
#ifndef SFRAME_PROXY_SERVICE_H
#define SFRAME_PROXY_SERVICE_H

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "AdminCmd.h"
#include "ServiceSession.h"
#include "Service.h"
#include "../util/RingQueue.h"
#include "ProxyServiceMsg.h"

namespace sframe {

// 代理服务
class ProxyService : public Service
{
public:

	static const std::string kAdminAddrDescName;



	ProxyService();

	virtual ~ProxyService();

	void Init() override;

	void OnDestroy() override;

	bool IsDestroyCompleted() const override;

	int32_t GetCyclePeriod() const override
	{
		return 1000;
	}

	// 处理周期定时器
	void OnCycleTimer() override;

	// 新连接到来
	void OnNewConnection(const ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock) override;

	// 代理服务消息
	void OnProxyServiceMessage(const std::shared_ptr<ProxyServiceMessage> & msg) override;

	// 注册会话
	// 返回会话ID（大于0的整数），否则为失败
	int32_t RegistSession(int32_t sid, const std::string & remote_ip, uint16_t remote_port);

	// 注册管理命令处理方法
	void RegistAdminCmd(const std::string & cmd, const AdminCmdHandleFunc & func);

private:

	int32_t GetNewSessionId();

	ServiceSession * GetServiceSession(int32_t session_id);

	void AddServiceSession(int32_t session_id, ServiceSession * session);

	void DeleteServiceSession(int32_t session_id);

	void OnMsg_SessionClosed(bool by_self, int32_t session_id);

	void OnMsg_SessionRecvData(int32_t session_id, const std::shared_ptr<std::vector<char>> & data);

	void OnMsg_SessionConnectCompleted(int32_t session_id, bool success);

	void OnMsg_AdminCommand(int32_t admin_session_id, const std::shared_ptr<sframe::HttpRequest> & http_req);

	void OnMsg_SendAdminCommandResponse(int32_t admin_session_id, const std::string & data, const  std::shared_ptr<sframe::HttpRequest> & http_req);

private:

	static const int kQuickFindSessionArrLen = 512;

	ServiceSession * _quick_find_session_arr[kQuickFindSessionArrLen];          // 将session_id小于kQuickFindSessionArrLen的session复制一份，用于快速查找
	std::unordered_map<int32_t, ServiceSession*> _all_sessions;                 // 所有ServiceSession
	bool _have_no_session;
	std::unordered_map<int64_t, int32_t> _session_addr_to_sessionid;            // 对于主动连接的Session，目标地址到sessionid的映射
	std::unordered_map<int32_t, int32_t> _sid_to_sessionid;                     // 远程服务ID映射到会话ID
	std::unordered_map<int32_t, std::unordered_set<int32_t>> _sessionid_to_sid; // 会话ID映射到服务ID
	bool _listening;                                                            // 是否正在监听
	TimerManager _timer_mgr;                                                    // 定时器管理
	int32_t _cur_max_session_id;
	bool _session_id_first_loop;
	std::unordered_map<std::string, AdminCmdHandleFunc> _map_admin_cmd_func;    // 管理命令处理方法
};

}

#endif
