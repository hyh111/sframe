
#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <assert.h>
#include <vector>
#include "net/net.h"
#include "util/Singleton.h"
#include "util/Lock.h"

class ClientManager : public sframe::singleton<ClientManager>, public sframe::noncopyable, public sframe::TcpAcceptor::Monitor
{
public:
	// 连接通知
	void OnAccept(std::shared_ptr<sframe::TcpSocket> socket, sframe::Error err) override;
	// 停止
	void OnClosed(sframe::Error err) override;

public:
	ClientManager() {}
	~ClientManager() {}

	// 更新网关服务的当前连接数
	void UpdateGateServiceInfo(int32_t sid, int32_t cur_session_num);

	// 开始
	bool Start(const std::string & ip, uint16_t port);

private:
	// 选择一个Gate服务
	int32_t ChooseGate();

private:

	struct GateServiceInfo
	{
		bool operator < (const GateServiceInfo & obj) const
		{
			if (session_num == obj.session_num)
			{
				return sid < obj.sid;
			}

			return session_num < obj.session_num;
		}

		int32_t sid;
		int32_t session_num;
	};

	std::shared_ptr<sframe::TcpAcceptor> _acceptor;
	std::vector<GateServiceInfo> _gate_info;
	sframe::Lock _lock;
};

#endif
