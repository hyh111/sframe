
#include <sstream>
#include "../net/IoService.h"
#include "ServiceDispatcher.h"
#include "Service.h"
#include "ProxyService.h"
#include "Listener.h"
#include "../util/TimeHelper.h"
#include "../util/Log.h"

using namespace sframe;

static const int32_t kMaxWaitMiliseconds = 20000;

// IO线程函数
void ServiceDispatcher::ExecIO(ServiceDispatcher * dispatcher)
{
	try
	{
		int64_t min_next_timer_time = 0;

		while (dispatcher->_ioservice->IsOpen())
		{
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

			int64_t wait_timeout_milisec = kMaxWaitMiliseconds;
			if (min_next_timer_time > 0)
			{
				int64_t next_timer_after_millisec = min_next_timer_time > now ? min_next_timer_time - now : 0;
				wait_timeout_milisec = wait_timeout_milisec > next_timer_after_millisec ? next_timer_after_millisec : wait_timeout_milisec;
			}
			
			Error err = ErrorSuccess;
			dispatcher->_ioservice->RunOnce((int32_t)wait_timeout_milisec, err);
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
			Service * s = nullptr;
			if (dispatcher->_dispach_service_queue.Pop(&s))
			{
				if (s)
				{
					s->Process();
				}
				else
				{
					assert(false);
				}
			}
        }
    }
    catch (const std::exception& e)
	{
		LOG_ERROR << "Logic thread(" << std::this_thread::get_id() << ") catched exception and exited|sid|" << cur_sid << "|error|" << e.what() << std::endl;
    }
}


ServiceDispatcher::ServiceDispatcher() : _running(false), _io_thread(nullptr), _dispach_service_queue(128, 16)
{
	memset(_services_arr, 0, sizeof(_services_arr));
	_ioservice = IoService::Create();
	assert(_ioservice);
}

ServiceDispatcher::~ServiceDispatcher()
{
	for (auto & it : _all_service)
	{
		if (it.second)
		{
			delete it.second;
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
void ServiceDispatcher::SendMsg(Service * s, const std::shared_ptr<Message> & msg)
{
	if (s)
	{
		s->PushMsg(msg);
	}
}

// 发消息
void ServiceDispatcher::SendMsg(int32_t sid, const std::shared_ptr<Message> & msg)
{
	Service *s = GetService(sid);
	SendMsg(s, msg);
}

// 设置远程服务监听地址
void ServiceDispatcher::SetServiceListenAddr(const std::string & ip, uint16_t port)
{
	RepareProxyServer();
	std::shared_ptr<ServiceTcpConnHandler> conn_handler = std::make_shared<ServiceTcpConnHandler>();
	conn_handler->SetHandleServices(std::set<int32_t>{0});
	Listener * listener = new Listener(ip, port, "ServConnectAddr", conn_handler);
	assert(listener);
	_listeners.push_back(listener);
}

// 设置服务器管理监听地址
void ServiceDispatcher::SetAdminListenAddr(const std::string & ip, uint16_t port)
{
	RepareProxyServer();
	std::shared_ptr<ServiceTcpConnHandler> conn_handler = std::make_shared<ServiceTcpConnHandler>();
	conn_handler->SetHandleServices(std::set<int32_t>{0});
	Listener * listener = new Listener(ip, port, "AdminAddr", conn_handler);
	assert(listener);
	_listeners.push_back(listener);
}

// 设置自定义监听地址
bool ServiceDispatcher::SetCustomListenAddr(const std::string & desc_name, const std::string & ip, uint16_t port, const std::set<int32_t> & handle_services, ConnDistributeStrategy * distribute_strategy)
{
	if (handle_services.empty())
	{
		return false;
	}

	for (auto it = handle_services.begin(); it != handle_services.end(); it++)
	{
		if (*it <= 0)
		{
			return false;
		}
	}

	std::shared_ptr<ServiceTcpConnHandler> conn_handler = std::make_shared<ServiceTcpConnHandler>();
	conn_handler->SetHandleServices(handle_services);
	Listener * listener = new Listener(ip, port, desc_name, conn_handler);
	assert(listener);
	_listeners.push_back(listener);

	return true;
}

// 设置自定义监听地址
bool ServiceDispatcher::SetCustomListenAddr(const std::string & desc_name, const std::string & ip, uint16_t port, int32_t handle_service)
{
	if (handle_service <= 0)
	{
		return false;
	}

	std::shared_ptr<ServiceTcpConnHandler> conn_handler = std::make_shared<ServiceTcpConnHandler>();
	conn_handler->SetHandleServices(std::set<int32_t>{handle_service});
	Listener * listener = new Listener(ip, port, "ServConnectAddr", conn_handler);
	assert(listener);
	_listeners.push_back(listener);

	return true;
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
	for (auto & pr_service : _all_service)
	{
		Service * s = pr_service.second;
		if (s != nullptr)
		{
			// 初始化
			s->Init();
			// 设置周期
			int32_t period = s->GetCyclePeriod();
			if (period > 0)
			{
				_cycle_timers.push_back(new CycleTimer(s->GetServiceId(), period));
			}
		}
		else
		{
			assert(false);
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
	for (auto & pr_service : _all_service)
	{
		Service * s = pr_service.second;
		if (s != nullptr)
		{
			int32_t priority = s->GetDestroyPriority();
			priority = priority > 0 ? priority : 0;
			destroy_priority_to_service[priority].push_back(s);
		}
		else
		{
			assert(false);
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
void ServiceDispatcher::Dispatch(Service * s)
{
	if (s)
	{
		_dispach_service_queue.Push(s);
	}
	else
	{
		assert(false);
	}
}

// 注册工作服务
bool ServiceDispatcher::RegistService(int32_t sid, Service * service)
{
	if (_running || !service || sid == 0 || _all_service.find(sid) != _all_service.end())
	{
		return false;
	}

	service->SetServiceId(sid);
	_all_service[sid] = service;
	// 若ID在[1,kServiceArrLen)区间中，拷贝一份在_service_arr
	if (sid > 0 && sid < kServiceArrLen)
	{
		_services_arr[sid] = service;
	}

	return true;
}

// 注册远程服务
bool ServiceDispatcher::RegistRemoteService(int32_t sid, const std::string & remote_ip, uint16_t remote_port)
{
	if (sid <= 0 || remote_ip.empty())
	{
		return false;
	}

	ProxyService* proxy_service = (ProxyService*)RepareProxyServer();
	int32_t session_id = proxy_service->RegistSession(sid, remote_ip, remote_port);
	if (session_id <= 0)
	{
		return false;
	}

	return true;
}

// 注册管理命令处理方法
void ServiceDispatcher::RegistAdminCmd(const std::string & cmd, const AdminCmdHandleFunc & func)
{
	if (cmd.empty())
	{
		return;
	}

	ProxyService* proxy_service = (ProxyService*)RepareProxyServer();
	proxy_service->RegistAdminCmd(cmd, func);
}

// 指定服务ID是否是本地服务
bool ServiceDispatcher::IsLocalService(int32_t sid) const
{
	return (sid != 0 && GetService(sid) != nullptr);
}

// 准备代理服务
Service * ServiceDispatcher::RepareProxyServer()
{
	if (_services_arr[0] == nullptr)
	{
		assert(_all_service.find(0) == _all_service.end());
		_services_arr[0] = new ProxyService();
		assert(_services_arr[0]);
		_all_service[0] = _services_arr[0];
	}

	return _services_arr[0];
}

// 获取服务
Service * ServiceDispatcher::GetService(int32_t sid) const
{
	if (sid >= 0 && sid < kServiceArrLen)
	{
		return _services_arr[sid];
	}

	auto it = _all_service.find(sid);
	if (it != _all_service.end())
	{
		return it->second;
	}

	return nullptr;
}
