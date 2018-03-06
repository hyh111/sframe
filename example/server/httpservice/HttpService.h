
#ifndef __HTTP_SERVICE_H__
#define __HTTP_SERVICE_H__

#include <inttypes.h>
#include <unordered_map>
#include "../ssproto/SSMsg.h"
#include "serv/Service.h"
#include "util/ObjectFactory.h"
#include "HttpSession.h"

class HttpService : public sframe::Service, public sframe::RegFactoryByName<HttpService>
{
public:

	HttpService() : _max_session_id(0) {}

	~HttpService() {}

	// ³õÊ¼»¯£¨´´½¨·þÎñ³É¹¦ºóµ÷ÓÃ£¬´ËÊ±»¹Î´¿ªÊ¼ÔËÐÐ£©
	void Init() override;

	// ÐÂÁ¬½Óµ½À´
	void OnNewConnection(const sframe::ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock) override;

	// ´¦ÀíÏú»Ù
	void OnDestroy() override;

	// ÊÇ·ñÏú»ÙÍê³É
	bool IsDestroyCompleted() const override
	{
		return _sessions.empty();
	}

	HttpSession * GetHttpSession(int64_t session_id) const;

	void RemoveHttpSession(int64_t session_id);

private:
	std::unordered_map<int64_t, std::shared_ptr<HttpSession>> _sessions;
	int64_t _max_session_id;
};

#endif
