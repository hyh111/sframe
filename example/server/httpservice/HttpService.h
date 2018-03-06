
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

	// 初始化（创建服务成功后调用，此时还未开始运行）
	void Init() override;

	// 新连接到来
	void OnNewConnection(const sframe::ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock) override;

	// 处理销毁
	void OnDestroy() override;

	// 是否销毁完成
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
