
#include <sstream>
#include "../net/IoService.h"
#include "ServiceDispatcher.h"
#include "Service.h"
#include "ProxyService.h"
#include "Listener.h"
#include "../util/TimeHelper.h"
#include "../util/Log.h"

using namespace sframe;

// IO线程函数
void ServiceDispatcher::ExecIO(ServiceDispatcher * dispatcher)
{
	try
	{
		int64_t min_next_timer_time = 0;

		while (dispatcher->_ioservice->IsOpen())
		{
			int64_t wait_timeout = 20000;
			int64_t now = TimeHelper::GetSteadyMiliseconds();

			// 检测定时器
			if (now >= min_next_timer_time && !dispatcher->_cycle_timers.empty())
			{
				min_next_timer_time = 0;
				for (CycleTimer * cur : dispatcher->_cycle_timers)
				{
					if (now >= cur->next_time)
					{
						// 发送周期消息到目标服务
						if (cur->msg->TryLock())
						{
							std::shared_ptr<Message> cycle_msg(cur->msg);
							dispatcher->SendMsg(cur->sid, cycle_msg);
							// 调整下一次执行时间
							cur->next_time = now + cur->msg->GetPeriod();
						}
					}

					// 刷新最小的下次执行时间
					if (min_next_timer_time <= 0 || cur->next_time < min_next_timer_time)
					{
						min_next_timer_time = cur->next_time;
					}
				}
			}

			if (min_next_timer_time > 0)
			{
				int64_t next_timer_after_millisec = min_next_timer_time - now;
				assert(next_timer_after_millisec > 0);
				wait_timeout = wait_timeout > next_timer_after_millisec ? next_timer_after_millisec : wait_timeout;
			}
			
			Error err = ErrorSuccess;
			dispatcher->_ioservice->RunOnce((int32_t)wait_timeout, err);
			if (err)
			{
				LOG_ERROR << "Run IoService error: " << ErrorMessage(err).Message() << ENDL;
			}
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << "IO thread catched exception and exited|error|" << e.what() << std::endl;
	}
}

// 业务线程函数
void ServiceDispatcher::ExecWorker(ServiceDispatcher * dispatcher)
{
	int32_t cur_sid = 0;

    try
    {
        while (dispatcher->_running)
        {
			// 处理服务消息
			cur_sid = -1;
			if (dispatcher->_dispach_service_queue.Pop(&cur_sid))
			{
				assert(cur_sid >= 0 && cur_sid <= kMaxServiceId && dispatcher->_services[cur_sid]);
				dispatcher->_services[cur_sid]->Process();
			}
        }
    }
    catch (const std::exception& e)
	{
		LOG_ERROR << "Logic thread(" << std::this_thread::get_id() << ") catched exception and exited|sid|" << cur_sid << "|error|" << e.what() << std::endl;
    }
}


ServiceDispatcher::ServiceDispatcher() : _max_sid(0), _running(false), _io_thread(nullptr), _dispach_service_queue(kMaxServiceId + 1)
{
	memset(_services, 0, sizeof(_services));
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

    for (auto t : _logic_threads)
    {
        delete t;
    }

	if (_io_thread)
	{
		delete _io_thread;
	}

	for (auto cycle_timer : _cycle_timers)
	{
		delete cycle_timer;
	}
}

// 发消息
void ServiceDispatcher::SendMsg(int32_t sid, const std::shared_ptr<Message> & msg)
{
	if (sid >= 0 && sid <= kMaxServiceId && _services[sid])
	{
		_services[sid]->PushMsg(msg);
	}
}

// 设置远程服务监听地址
void ServiceDispatcher::SetServiceListenAddr(const std::string & ip, uint16_t port)
{
	RepareProxyServer();
	Listener * listener = new Listener(ip, port, 0);
	assert(listener);
	listener->SetDescName("ServiceConnectAddr");
	_listeners.push_back(listener);
}

// 设置自定义监听地址
void ServiceDispatcher::SetCustomListenAddr(const std::string & desc_name, const std::string & ip, uint16_t port, const std::set<int32_t> & handle_services, ConnDistributeStrategy * distribute_strategy)
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

	Listener * listener = new Listener(ip, port, handle_services, distribute_strategy);
	assert(listener);
	listener->SetDescName(desc_name);
	_listeners.push_back(listener);
}

// 设置自定义监听地址
void ServiceDispatcher::SetCustomListenAddr(const std::string & desc_name, const std::string & ip, uint16_t port, int32_t handle_service)
{
	if (handle_service <= 0)
	{
		return;
	}

	Listener * listener = new Listener(ip, port, handle_service);
	assert(listener);
	listener->SetDescName(desc_name);
	_listeners.push_back(listener);
}

// 开始
bool ServiceDispatcher::Start(int32_t thread_num)
{
	if (_running || thread_num <= 0 || !_ioservice)
	{
		assert(false);
		return false;
	}

	Error err = _ioservice->Init();
	if (err)
	{
		LOG_ERROR << "Initialize IoService error|" << err.Code() << "|" << ErrorMessage(err).Message() << ENDL;
		return false;
	}

	_running = true;

	// 初始化所有服务，并设置循环周期
	for (int i = 0; i <= _max_sid; i++)
	{
		if (_services[i] != nullptr)
		{
			// 初始化
			_services[i]->Init();
			// 设置周期
			int32_t period = _services[i]->GetCyclePeriod();
			if (period > 0)
			{
				_cycle_timers.push_back(new CycleTimer(i, period));
			}
		}
	}

	// 开启所有监听器
	for (auto it = _listeners.begin(); it < _listeners.end(); it++)
	{
		(*it)->Start();
	}

	// 开启IO线程
	assert(!_io_thread);
	_io_thread = new std::thread(ServiceDispatcher::ExecIO, this);

    // 开启逻辑线程
    for (int i = 0; i < thread_num; i++)
    {
		std::thread * t = new std::thread(ServiceDispatcher::ExecWorker, this);
        _logic_threads.push_back(t);
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

	// 确定所有服务的销毁优先级批次
	std::map<int32_t, std::vector<Service*>> destroy_priority_to_service;
	for (int i = 0; i <= _max_sid; i++)
	{
		if (_services[i] != nullptr)
		{
			int32_t priority = _services[i]->GetDestroyPriority();
			priority = priority > 0 ? priority : 0;
			destroy_priority_to_service[priority].push_back(_services[i]);
		}
	}

	// 销毁服务
	std::shared_ptr<Message> destroy_msg = std::make_shared<DestroyServiceMessage>();
	for (auto it = destroy_priority_to_service.begin(); it != destroy_priority_to_service.end(); it++)
	{
		// 发送销毁消息
		for (Service * s : it->second)
		{
			if (s)
			{
				s->PushMsg(destroy_msg);
			}
			else
			{
				assert(false);
			}
		}

		// 等待销毁完成
		for (Service * s : it->second)
		{
			s->WaitDestroyComplete();
		}
	}

    _running = false;
	_dispach_service_queue.Stop();
    for (std::thread * t : _logic_threads)
    {
        t->join();
        delete t;
    }
	_logic_threads.clear();

	// 停止IO服务和IO线程
	_ioservice->Close();
	_io_thread->join();
	delete _io_thread;
	_io_thread = nullptr;
}

// 调度服务(将指定服务压入调度队列)
void ServiceDispatcher::Dispatch(int32_t sid)
{
	assert(sid >= 0 && sid <= kMaxServiceId && _services[sid]);
	_dispach_service_queue.Push(sid);
}

// 注册工作服务
bool ServiceDispatcher::RegistService(int32_t sid, Service * service)
{
	if (_running || !service || sid < 1 || sid > kMaxServiceId || _services[sid])
	{
		return false;
	}

	service->SetServiceId(sid);
	_max_sid = sid > _max_sid ? sid : _max_sid;
	_local_sid.push_back(sid);
	_services[sid] = service;

	return true;
}

// 注册远程服务
bool ServiceDispatcher::RegistRemoteService(int32_t sid, const std::string & remote_ip, uint16_t remote_port)
{
	if (sid <= 0 || remote_ip.empty())
	{
		return false;
	}

	RepareProxyServer();

	int32_t session_id = ((ProxyService*)_services[0])->RegistSession(sid, remote_ip, remote_port);
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
	}
}
