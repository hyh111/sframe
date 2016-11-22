
#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

#include <inttypes.h>
#include <memory>
#include "net/net.h"
#include "util/Http.h"

class HttpService;

class HttpSession : public sframe::TcpSocket::Monitor
{
public:

	HttpSession(HttpService * http_service, const std::shared_ptr<sframe::TcpSocket> & sock, int64_t session_id);

	~HttpSession() {}

	// 接收到数据
	// 返回剩余多少数据
	int32_t OnReceived(char * data, int32_t len) override;

	// Socket关闭
	// by_self: true表示主动请求的关闭操作
	void OnClosed(bool by_self, sframe::Error err) override;

	void StartClose();

	int64_t GetSessionId() const
	{
		return _session_id;
	}

	void OnMsg_HttpRequest(std::shared_ptr<sframe::HttpRequest> http_req);

	void OnMsg_HttpSessionClosed();

private:
	HttpService * _http_service;
	std::shared_ptr<sframe::TcpSocket> _sock;
	int64_t _session_id;
	sframe::HttpDecoder _http_decoder;
};

#endif