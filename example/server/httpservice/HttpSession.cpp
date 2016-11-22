
#include "HttpSession.h"
#include "HttpService.h"
#include "util/Log.h"

HttpSession::HttpSession(HttpService * http_service, const std::shared_ptr<sframe::TcpSocket> & sock, int64_t session_id)
	: _http_service(http_service), _sock(sock), _session_id(session_id), _http_decoder(sframe::kHttpType_Request)
{
	_sock->SetMonitor(this);
	_sock->StartRecv();
}

// 接收到数据
// 返回剩余多少数据
int32_t HttpSession::OnReceived(char * data, int32_t len)
{
	std::string err_msg;
	size_t readed = _http_decoder.Decode(data, len, err_msg);
	if (!err_msg.empty())
	{
		FLOG("HttpService") << "HttpSession " << _session_id << " decode http request error|" << err_msg << std::endl;
		// 关闭连接
		return -1;
	}

	if (_http_decoder.IsDecodeCompleted())
	{
		std::shared_ptr<sframe::HttpRequest> http_req = _http_decoder.GetHttpRequest();
		_http_decoder.Reset();
		assert(http_req);
		_http_service->SendInsideServiceMsg(_http_service->GetServiceId(), _session_id, kHttpMsg_HttpRequest, http_req);
	}

	return len - (int32_t)readed;
}

// Socket关闭
// by_self: true表示主动请求的关闭操作
void HttpSession::OnClosed(bool by_self, sframe::Error err)
{
	if (err)
	{
		FLOG("HttpService") << "HttpSession " << _session_id << " closed with error|" << sframe::ErrorMessage(err).Message() << std::endl;
	}
	else
	{
		FLOG("HttpService") << "HttpSession " << _session_id << " closed succ" << std::endl;
	}

	_http_service->SendInsideServiceMsg(_http_service->GetServiceId(), _session_id, kHttpMsg_HttpSessionClosed);
}

void HttpSession::StartClose()
{
	assert(_sock);
	_sock->Close();
}

void HttpSession::OnMsg_HttpRequest(std::shared_ptr<sframe::HttpRequest> http_req)
{
	std::string req_method = http_req->GetMethod();
}

void HttpSession::OnMsg_HttpSessionClosed()
{
	_http_service->RemoveHttpSession(_session_id);
}