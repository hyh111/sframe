
#pragma once
#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <atomic>
#include <unordered_map>
#include "util/Singleton.h"
#include "util/Timer.h"
#include "Client.h"

class ClientManager : public sframe::singleton<ClientManager>
{
public:

	ClientManager() : _timer_mgr(std::bind(&ClientManager::GetClient, this, std::placeholders::_1))
	{
		_state.store(1);
	}

	bool Init();

	int32_t RegistTimer(int32_t client_id, int32_t after_ms, sframe::ObjectTimerManager<int32_t, Client>::TimerFunc func);

	void Update();

	void Close();

	Client * GetClient(int32_t id);

	void CloseClient(int32_t id);

	const std::shared_ptr<sframe::IoService> & GetIoService() const
	{
		return _ioservice;
	}

private:
	std::atomic_int _state;
	sframe::ObjectTimerManager<int32_t, Client> _timer_mgr;
	std::unordered_map<int32_t, std::shared_ptr<Client>> _clients;
	std::shared_ptr<sframe::IoService> _ioservice;
};

#endif