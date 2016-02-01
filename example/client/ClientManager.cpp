
#include <assert.h>
#include "util/TimeHelper.h"
#include "util/Log.h"
#include "ClientManager.h"
#include "ConfigManager.h"

using namespace sframe;

bool ClientManager::Init()
{
	auto config = ConfigManager::Instance().GetMapConfig<ClientConfig::KeyType, ClientConfig>();
	if (config == nullptr)
	{
		return false;
	}

	for (auto item : *config)
	{
		std::shared_ptr<Client> c = std::make_shared<Client>(item.second->client_id, this);
		if (c == nullptr)
		{
			continue;
		}

		_clients.insert(std::make_pair(c->GetId(), c));
	}

	_ioservice = sframe::IoService::Create();
	assert(_ioservice);
	sframe::Error err = _ioservice->Init();
	if (err)
	{
		LOG_ERROR << "Init IoService error(" << err.Code() << "):" << sframe::ErrorMessage(err).Message() << ENDL;
		return false;
	}

	for (auto item : _clients)
	{
		item.second->Init();
	}

	return true;
}

// 注册会话定时器
int32_t ClientManager::RegistTimer(int32_t client_id, int32_t after_ms, sframe::ObjectTimerManager<int32_t, Client>::TimerFunc func)
{
	int64_t cur_time = sframe::TimeHelper::GetEpochMilliseconds();
	return _timer_mgr.Regist(cur_time + after_ms, client_id, func);
}

void ClientManager::Update()
{
	Error err = ErrorSuccess;
	_ioservice->RunOnce(200, err);
	if (err)
	{
		LOG_ERROR << "Run IoService error: " << ErrorMessage(err).Message() << ENDL;
		return;
	}

	int s = _state.load();
	if (s == 1)
	{
		int64_t cur_time = TimeHelper::GetEpochMilliseconds();
		_timer_mgr.Execute(cur_time);
	}
	else if (s == 2)
	{
		for (auto item : _clients)
		{
			item.second->Close();
		}

		_state.store(3);
	}
}

void ClientManager::Close()
{
	_state.store(2);

	while (!_clients.empty())
	{
		TimeHelper::ThreadSleep(100);
	}
}

Client * ClientManager::GetClient(int32_t id)
{
	auto it = _clients.find(id);
	if (it == _clients.end())
	{
		return nullptr;
	}

	return it->second.get();
}

void ClientManager::CloseClient(int32_t id)
{
	_clients.erase(id);
}