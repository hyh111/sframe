
#include "../net/IoService.h"
#include "ServiceDispatcher.h"
#include "Service.h"
#include "ProxyService.h"
#include "Listener.h"
#include "../util/TimeHelper.h"
#include "../util/Log.h"

using namespace sframe;

// 业务线程函数
void ServiceDispatcher::ExecWorker(ServiceDispatcher * dispatcher)
{
	uint32_t step = 0;
	int32_t cur_sid = 0;

    try
    {
        while (true)
        {
			// 运行一次IO服务
			step = 1;
			Error err = ErrorSuccess;
			dispatcher->_ioservice->RunOnce(1, err);
			if (err)
			{
				LOG_ERROR << "Run IoService error: " << ErrorMessage(err).Message() << ENDL;
			}

			// 运行timer
			step = 2;
			bool cmp = false;
			if (dispatcher->_checking_timer.compare_exchange_strong(cmp, true))
			{
				int64_t now = TimeHelper::GetEpochMilliseconds();

				for (CycleTimer & cycle_timer : dispatcher->_cycle_timers)
				{
					if (now < cycle_timer.next_time)
					{
						continue;
					}

					if (cycle_timer.msg->TryLock())
					{
						std::shared_ptr<Message> cycle_msg(cycle_timer.msg);
						dispatcher->SendMsg(cycle_timer.sid, cycle_msg);
						cycle_timer.next_time = now + cycle_timer.msg->GetPeriod();
					}
				}

				dispatcher->_checking_timer.store(false);
			}

			// 处理服务消息
			cur_sid = 0;
			if (dispatcher->_dispach_service_queue.Pop(&cur_sid))
			{
				assert(cur_sid >= 0 && cur_sid <= kMaxServiceId && dispatcher->_services[cur_sid]);
				dispatcher->_services[cur_sid]->Process();
			}
			else if (!dispatcher->_running)
			{
				break;
			}
        }
    }
    catch (const std::runtime_error& e)
    {
		static const char kStepName[][20] = { "None", "IoService", "CheckTimer", "LogicService" };
		LOG_ERROR << "Thread exited, catched exception when run " << kStepName[step > 3 ? 0 : step] 
			<< ", LogicService(" << cur_sid << "), Error: " << e.what() << ENDL;
    }
}


ServiceDispatcher::ServiceDispatcher() : _max_sid(0), _running(false)
{
	memset(_services, 0, sizeof(_services));
	_checking_timer.store(false);
	_ioservice = IoService::Create();
	assert(_ioservice);
}

ServiceDispatcher::~ServiceDispatcher()
{
    for (int i = 0; i <= _max_sid; i++)
    {
        if (_services[i])
        {
            delete _services[i];
        }
    }

    for (std::thread * t : _threads)
    {
        delete t;
    }
}

// 发消息
void ServiceDispatcher::SendMsg(int32_t sid, const std::shared_ptr<Message> & msg)
{
	if (_running && sid >= 0 && sid <= kMaxServiceId && _services[sid])
	{
		_services[sid]->PushMsg(msg);
	}
}

// 设置远程服务监听地址
void ServiceDispatcher::SetServiceListenAddr(const std::string & ipv4, uint16_t port, const std::string & key)
{
	RepareProxyServer();
	((ProxyService*)_services[0])->SetLocalAuthKey(key);
	Listener * listener = new Listener(ipv4, port, 0);
	assert(listener);
	_listeners.push_back(listener);
}

// 设置自定义监听地址
void ServiceDispatcher::SetCustomListenAddr(const std::string & ipv4, uint16_t port, const std::set<int32_t> & handle_services, ConnDistributeStrategy * distribute_strategy)
{
	if (handle_services.empty())
	{
		return;
	}

	for (auto it = handle_services.begin(); it != handle_services.end(); it++)
	{
		if (*it <= 0)
		{
			return;
		}
	}

	Listener * listener = new Listener(ipv4, port, handle_services, distribute_strategy);
	assert(listener);
	_listeners.push_back(listener);
}

// 设置自定义监听地址
void ServiceDispatcher::SetCustomListenAddr(const std::string & ipv4, uint16_t port, int32_t handle_service)
{
	if (handle_service <= 0)
	{
		return;
	}

	Listener * listener = new Listener(ipv4, port, handle_service);
	assert(listener);
	_listeners.push_back(listener);
}

// 开始
bool ServiceDispatcher::Start(int32_t thread_num)
{
	assert(!_running && thread_num > 0 && _ioservice);

	Error err = _ioservice->Init();
	if (err)
	{
		LOG_ERROR << "Initialize IoService error(" << err.Code() << "): " << ErrorMessage(err).Message() << ENDL;
		return false;
	}

	// 初始化所有服务
	for (int i = 0; i <= _max_sid; i++)
	{
		if (_services[i] != nullptr)
		{
			_services[i]->Init();
		}
	}

    _running = true;

	// 开启所有监听器
	for (auto it = _listeners.begin(); it < _listeners.end(); it++)
	{
		(*it)->Start();
	}

	// 通知所有服务开始
	std::unordered_set<int32_t> usable_service;
	std::shared_ptr<Message> start_msg = std::make_shared<StartServiceMessage>();
	for (int i = 0; i <= _max_sid; i++)
	{
		if (_services[i] != nullptr)
		{
			usable_service.insert(i);
			_services[i]->PushMsg(start_msg);
		}
	}

	// 通知所有服务，服务可用
	NotifyServiceJoin(usable_service, false);

    // 开启工作线程
    for (int i = 0; i < thread_num; i++)
    {
        std::thread * t = new std::thread(ServiceDispatcher::ExecWorker, this);
        _threads.push_back(t);
    }

	return true;
}

// 停止
void ServiceDispatcher::Stop()
{
	// 停止所有监听器
	for (auto it = _listeners.begin(); it < _listeners.end(); it++)
	{
		(*it)->Stop();
	}
	for (auto it = _listeners.begin(); it < _listeners.end(); it++)
	{
		while ((*it)->IsRunning())
		{
			TimeHelper::ThreadSleep(1);
		}
	}

	// 给所有服务发送销毁消息
	std::shared_ptr<Message> destroy_msg = std::make_shared<DestroyServiceMessage>();
	for (int i = 0; i <= _max_sid; i++)
	{
		if (_services[i] != nullptr)
		{
			_services[i]->PushMsg(destroy_msg);
		}
	}

	// 等待所有服务销毁完成
	for (int i = 0; i <= _max_sid; i++)
	{
		if (_services[i] != nullptr)
		{
			_services[i]->WaitDestroyComplete();
		}
	}

    _running = false;

    for (std::thread * t : _threads)
    {
        t->join();
        delete t;
    }

    _threads.clear();
}

// 调度服务(将指定服务压入调度队列)
void ServiceDispatcher::Dispatch(int32_t sid)
{
	assert(sid >= 0 && sid <= kMaxServiceId && _services[sid]);
	_dispach_service_queue.Push(sid);
}

// 通知所有本地服务，其他服务的接入
void ServiceDispatcher::NotifyServiceJoin(const std::unordered_set<int32_t> & service_set, bool is_remote)
{
	for (int32_t target_sid : _local_sid)
	{
		if (_services[target_sid] != nullptr)
		{
			std::shared_ptr<ServiceJoinMessage> state_msg = std::make_shared<ServiceJoinMessage>();
			
			for (int32_t sid : service_set)
			{
				if (sid != target_sid && _services[target_sid]->IsCareServiceJoin(sid))
				{
					state_msg->service.insert(sid);
				}
			}

			if (!state_msg->service.empty())
			{
				state_msg->is_remote = is_remote;
				std::shared_ptr<Message> msg(state_msg);
				_services[target_sid]->PushMsg(msg);
			}
		}
	}
}

// 注册远程服务器
bool ServiceDispatcher::RegistRemoteServer(const std::string & remote_ip, uint16_t remote_port, const std::string & remote_key)
{
	if (remote_ip.empty())
	{
		assert(false);
		return false;
	}

	RepareProxyServer();

	int32_t session_id = ((ProxyService*)_services[0])->RegistSession(remote_ip, remote_port, remote_key);
	if (session_id <= 0)
	{
		return false;
	}

	return true;
}

// 准备代理服务
void ServiceDispatcher::RepareProxyServer()
{
	if (_services[0] == nullptr)
	{
		_services[0] = new ProxyService();
		assert(_services[0]);
		_cycle_timers.push_back(CycleTimer(0, 1000));
	}
}
