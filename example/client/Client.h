
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
	// ½ÓÊÕµ½Êý¾Ý
	// ·µ»ØÊ£Óà¶àÉÙÊý¾Ý
	int32_t OnReceived(char * data, int32_t len) override;

	// Socket¹Ø±Õ
	// by_self: true±íÊ¾Ö÷¶¯ÇëÇóµÄ¹Ø±Õ²Ù×÷
	void OnClosed(bool by_self, sframe::Error err) override;

	// Á¬½Ó²Ù×÷Íê³É
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