
#ifndef SFRAME_LISTENER_H
#define SFRAME_LISTENER_H

#include <assert.h>
#include <inttypes.h>
#include <atomic>
#include <string>
#include <set>
#include "../net/net.h"
#include "../util/Singleton.h"

namespace sframe {

// 连接派发策略
class ConnDistributeStrategy
{
public:
	ConnDistributeStrategy()
	{
		_it_cur_sid = _handle_services.begin();
	}

	virtual ~ConnDistributeStrategy() {}

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

	virtual int32_t DistributeHandleService(const std::shared_ptr<TcpSocket> & sock);

private:
	std::set<int32_t> _handle_services;
	std::set<int32_t>::iterator _it_cur_sid;
};

// 监听器
class Listener : public TcpAcceptor::Monitor, public noncopyable
{
public:

	Listener(const std::string & ip, uint16_t port, int32_t handle_service)
		: _ip(ip), _port(port)
	{
		_running.store(false);
		_distribute_strategy = new ConnDistributeStrategy();
		_distribute_strategy->SetHandleServices(std::set<int32_t> {handle_service});
	}

	Listener(const std::string & ip, uint16_t port, const std::set<int32_t> & handle_services, ConnDistributeStrategy * distribute_strategy = nullptr)
		: _ip(ip), _port(port)
	{
		_running.store(false);
		if (distribute_strategy == nullptr)
		{
			_distribute_strategy = new ConnDistributeStrategy();
		}
		_distribute_strategy->SetHandleServices(handle_services);
	}

	~Listener()
	{
		delete _distribute_strategy;
	}

	bool IsRunning() const
	{
		return _running.load();
	}

	bool Start();

	void Stop();

	// 连接通知
	void OnAccept(std::shared_ptr<TcpSocket> socket, Error err) override;

	// 停止
	void OnClosed(Error err) override;

private:
	std::string _ip;
	uint16_t _port;
	std::shared_ptr<TcpAcceptor> _acceptor;
	ConnDistributeStrategy * _distribute_strategy;
	std::atomic_bool _running;
};

}

#endif