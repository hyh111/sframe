
#ifndef SFRAME_SERVICE_LISTENER_H
#define SFRAME_SERVICE_LISTENER_H

#include <string>
#include "../net/net.h"
#include "../util/Singleton.h"

namespace sframe {

// 服务监听器（监听管理远程服务连接）
class ServiceListener : public TcpAcceptor::Monitor, public noncopyable
{
public:
	ServiceListener() : _listen_port(0) {}

	~ServiceListener() {}

	// 设置监听地址
	void SetListenAddr(const std::string & listen_ip, uint16_t listen_port)
	{
		_listen_ip = listen_ip;
		_listen_port = listen_port;
	}

	// 开始
	bool Start();

	// 停止
	void Stop();

	// 连接通知
	void OnAccept(std::shared_ptr<TcpSocket> socket, Error err) override;

	// 停止
	void OnClosed(Error err) override;

private:
	std::shared_ptr<TcpAcceptor> _acceptor;
	std::string _listen_ip;
	uint16_t _listen_port;
};

}

#endif
