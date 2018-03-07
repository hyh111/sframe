
#ifndef SFRAME_LISTENER_H
#define SFRAME_LISTENER_H

#include <assert.h>
#include <inttypes.h>
#include <atomic>
#include <string>
#include <set>
#include "../net/net.h"
#include "../util/Singleton.h"
#include "Message.h"

namespace sframe {

// TCP连接处理器
class TcpConnHandler
{
public:
	TcpConnHandler() {}

	virtual ~TcpConnHandler() {}

	virtual void HandleTcpConn(const std::shared_ptr<TcpSocket> & sock, const ListenAddress & listen_addr) = 0;
};

// 服务TCP连接处理器——将连接发送给指定的服务来处理
class ServiceTcpConnHandler : public TcpConnHandler
{
public:
	ServiceTcpConnHandler()
	{
		_it_cur_sid = _handle_services.begin();
	}

	~ServiceTcpConnHandler() {}

	void SetHandleServices(const std::set<int32_t> & handle_services)
	{
		assert(!handle_services.empty());
		_handle_services = handle_services;
		_it_cur_sid = _handle_services.begin();
	}

	const std::set<int32_t> & GetHandleServices() const
	{
		return _handle_services;
	}

	void HandleTcpConn(const std::shared_ptr<TcpSocket> & sock, const ListenAddress & listen_addr) override;

private:
	std::set<int32_t> _handle_services;
	std::set<int32_t>::iterator _it_cur_sid;
};

// 监听器
class Listener : public TcpAcceptor::Monitor, public noncopyable
{
public:

	Listener(const std::string & ip, uint16_t port, const std::string & desc_name, std::shared_ptr<TcpConnHandler> conn_handler);

	~Listener() {}

	bool Start();

	void Stop();

	// 连接通知
	void OnAccept(std::shared_ptr<TcpSocket> socket, Error err) override;

	// 停止
	void OnClosed(Error err) override;


	bool IsRunning() const
	{
		return _running;
	}

private:
	ListenAddress _addr;
	std::shared_ptr<TcpAcceptor> _acceptor;
	std::shared_ptr<TcpConnHandler> _conn_handler;
	bool _running;
};

}

#endif