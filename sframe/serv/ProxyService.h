
#ifndef SFRAME_PROXY_SERVICE_H
#define SFRAME_PROXY_SERVICE_H

#include <unordered_map>
#include <queue>
#include "ServiceSession.h"
#include "ServiceListener.h"
#include "Service.h"
#include "../util/Timer.h"
#include "ProxyServiceMsg.h"

namespace sframe {

// 代理服务
class ProxyService : public Service
{
public:
	static const int kMaxSessionNumber = 256;

public:

	ProxyService();

	virtual ~ProxyService();

	void Init() override;

	void OnStart() override;

	void OnDestroy() override;

	bool IsDestroyCompleted() const override;

	void SetListenAddr(const std::string & ipv4, uint16_t port, const std::string & key);

	// 处理周期定时器
	void OnCycleTimer() override;

	// 注册会话
	// 返回会话ID（大于0的整数），否则为失败
	int32_t RegistSession(const std::string & remote_ip, uint16_t remote_port, const std::string & remote_key);

	// 注册会话定时器
	int32_t RegistSessionTimer(int32_t session_id, int32_t after_ms, ObjectTimerManager<int32_t, ServiceSession>::TimerFunc func);

	// 开始会话
	void StartSession(int32_t session_id, const std::vector<int32_t> & remote_sid);

	void SetLocalAuthKey(const std::string & key)
	{
		_local_key = key;
	}

	const std::string & GetLocalAuthKey() const
	{
		return _local_key;
	}

private:

	int32_t GetNewSessionId();

	ServiceSession * GetServiceSessionById(int32_t session_id);

private:

	void OnMsg_SendToRemoteService(int32_t sid, std::shared_ptr<std::vector<char>> & data);

	void OnMsg_AddNewSession(std::shared_ptr<sframe::TcpSocket> & sock);

	void OnMsg_SessionClosed(int32_t session_id);

	void OnMsg_SessionRecvData(int32_t session_id, const std::shared_ptr<std::vector<char>> & data);

	void OnMsg_SessionConnectCompleted(int32_t session_id, bool success);

	void OnMsg_ServiceListenerClosed();

private:
	ServiceListener * _listener;
	ServiceSession * _session[kMaxSessionNumber + 1];
	std::queue<int32_t> _session_id_queue;
	int32_t _session_num;
	bool _listening;           // 是否正在监听
	ObjectTimerManager<int32_t, ServiceSession> _timer_mgr;
	std::string _local_key;    // 远程服务器连接到本服务器的认证的秘钥

	// 会话信息（包含会话ID和消息缓存）
	struct SessionInfo
	{
		SessionInfo() : session_id(0) {}

		int32_t session_id;
		std::vector<std::shared_ptr<std::vector<char>>> msg_cache;
	};

	std::unordered_map<int32_t, SessionInfo> _sid_to_sessioninfo;        // 服务ID到会话信息的映射
};

}

#endif
