
#pragma once
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <inttypes.h>
#include <memory>
#include <string>
#include "net/net.h"
#include "util/Timer.h"

class ClientManager;

class Client : public sframe::TcpSocket::Monitor, public sframe::SafeTimerRegistor<Client>
{
public:
	enum State : int32_t
	{
		kState_WaitConnect,
		kState_Connecting,
		kState_Working,

		kState_Count
	};

public:
	Client(int32_t id, ClientManager * mgr);

	~Client(){}

	void Init();

	int32_t GetId() const
	{
		return _id;
	}

	void Close();

public:
	// 接收到数据
	// 返回剩余多少数据
	int32_t OnReceived(char * data, int32_t len) override;

	// Socket关闭
	// by_self: true表示主动请求的关闭操作
	void OnClosed(bool by_self, sframe::Error err) override;

	// 连接操作完成
	void OnConnected(sframe::Error err) override;

private:
	int32_t OnTimer();

	void OnTimer_WaitConnect();

	void OnTimer_Working();

private:
	typedef void(Client::*TimerRouine)();
	static TimerRouine kStateRoutine[kState_Count];

	ClientManager * _mgr;
	int32_t _id;
	State _state;
	std::string _server_ip;
	uint16_t _server_port;
	std::shared_ptr<sframe::TcpSocket> _sock;
	int32_t _text_index;
	int32_t _count;
	std::string _log_name;
};

#endif