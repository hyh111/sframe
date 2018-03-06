
#include "HttpService.h"
#include "util/Log.h"

// ³õÊ¼»¯£¨´´½¨·þÎñ³É¹¦ºóµ÷ÓÃ£¬´ËÊ±»¹Î´¿ªÊ¼ÔËÐÐ£©
void HttpService::Init()
{
	std::function<HttpSession*(const int64_t &)> get_session_func = std::bind(&HttpService::GetHttpSession, this, std::placeholders::_1);
	RegistInsideServiceMessageHandler(kHttpMsg_HttpRequest, &HttpSession::OnMsg_HttpRequest, get_session_func);
	RegistInsideServiceMessageHandler(kHttpMsg_HttpSessionClosed, &HttpSession::OnMsg_HttpSessionClosed, get_session_func);
}

// ÐÂÁ¬½Óµ½À´
void HttpService::OnNewConnection(const sframe::ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock)
{
	sframe::Error err = sock->SetTcpNodelay(true);
	if (err)
	{
		FLOG("HttpService") << "Set tcp nodelay error|" << err.Code() << "|" << sframe::ErrorMessage(err).Message() << ENDL;
	}

	std::shared_ptr<HttpSession> http_session = std::make_shared<HttpSession>(this, sock, ++_max_session_id);
	_sessions.insert(std::make_pair(http_session->GetSessionId(), http_session));
	FLOG("HttpService") << "Build new session " << http_session->GetSessionId() << std::endl;
}

// ´¦ÀíÏú»Ù
void HttpService::OnDestroy()
{
	for (auto & pr : _sessions)
	{
		pr.second->StartClose();
	}
}

// »ñÈ¡HttpSession
HttpSession * HttpService::GetHttpSession(int64_t session_id) const
{
	auto it = _sessions.find(session_id);
	if (it == _sessions.end())
	{
		return nullptr;
	}

	return it->second.get();
}

void HttpService::RemoveHttpSession(int64_t session_id)
{
	_sessions.erase(session_id);
}