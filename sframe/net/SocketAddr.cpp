
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef __GNUC__
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include "SocketAddr.h"

using namespace sframe;

SocketAddr::SocketAddr(const char * ip_str, uint16_t port)
{
	_port = htons(port);
	inet_pton(AF_INET, ip_str, &_ip);
}

uint16_t SocketAddr::GetPortHost() const
{
	return ntohs(_port);
}

SocketAddrText::SocketAddrText(const SocketAddr & addr)
{
	uint32_t ip = addr.GetIp();
	inet_ntop(AF_INET, &ip, _text, sizeof(_text));
	int32_t ip_len = (int32_t)strlen(_text);
	assert(ip_len > 0 && ip_len <= 15);
	sprintf(_text + ip_len, ":%d", (int)addr.GetPortHost());
}