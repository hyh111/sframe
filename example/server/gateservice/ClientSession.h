
#ifndef __CLIENT_SESSION_H__
#define __CLIENT_SESSION_H__

#include <vector>
#include <memory>
#include "net/net.h"

class GateService;

class ClientSession : public sframe::TcpSocket::Monitor, public std::enable_shared_from_this<ClientSession>
{
public:
	// 接收到数据
	// 返回剩余多少数据
	int32_t OnReceived(char * data, int32_t len) override;

	// Socket关闭
	// by_self: true表示主动请求的关闭操作
	void OnClosed(bool by_self, sframe::Error err) override;

public:
	ClientSession(GateService * gate_service, int64_t session_id, const std::shared_ptr<sframe::TcpSocket> & sock);

	int64_t GetSessionId() const
	{
		return _session_id;
	}

	int32_t GetWorkService() const
	{
		return _cur_work_sid;
	}

	void StartClose();

	void EnterWorkService(int32_t sid);

	void HandleClosed();

	void SendToWorkService(const std::shared_ptr<std::vector<char>> & data);

	void SendToClient(const std::shared_ptr<std::vector<char>> & data);

private:
	GateService * _gate_service;
	std::shared_ptr<sframe::TcpSocket> _sock;
	int64_t _session_id;
	int32_t _cur_work_sid;
};

#endif
