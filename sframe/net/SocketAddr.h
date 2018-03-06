
#ifndef SFRAME_SOCKET_ADDR_H
#define SFRAME_SOCKET_ADDR_H

#include <inttypes.h>

namespace sframe {

// Socket地址封装
class SocketAddr
{
public:
	SocketAddr() : _ip(0), _port(0) {}

	SocketAddr(uint32_t ip, uint16_t port) : _ip(ip), _port(port) {}

	SocketAddr(const char * ip_str, uint16_t port);

	uint32_t GetIp() const
	{
		return _ip;
	}

	uint16_t GetPort() const
	{
		return _port;
	}

	uint16_t GetPortHost() const;

private:
	uint32_t _ip;
	uint16_t _port;
};


// 地址转换为文本
class SocketAddrText
{
public:
	SocketAddrText(const SocketAddr & addr);

	const char * Text() const
	{
		return _text;
	}

private:
	char _text[32];
};

}

#endif