
#ifndef SFRAME_SERVICE_H
#define SFRAME_SERVICE_H

#include <assert.h>
#include <vector>
#include <memory>
#include "../util/Lock.h"
#include "MessageDecoder.h"
#include "../util/Delegate.h"
#include "../util/Singleton.h"
#include "ServiceDispatcher.h"
#include "../net/net.h"

namespace sframe{

// 服务状态枚举
enum ServiceState : int32_t
{
	kServiceState_Idle = 1,       // 闲置
	kServiceState_WaitProcess,    // 等待处理
	kServiceState_Processing,     // 处理中
};

class Service;

// 服务消息队列
class MessageQueue : public noncopyable
{
public:
	MessageQueue(Service * service) : _related_service(service), _state(kServiceState_Idle)
	{
		assert(_related_service);
		_buf_write = new std::vector<std::shared_ptr<Message>>();
		_buf_read = new std::vector<std::shared_ptr<Message>>();
		_buf_write->reserve(1024);
		_buf_read->reserve(1024);
	}

	void Push(const std::shared_ptr<Message> & msg);

	std::vector<std::shared_ptr<Message>> * PopAll();

	void EndProcess();

private:
	Service * _related_service;
	Lock _lock;
	std::vector<std::shared_ptr<Message>> * _buf_write;
	std::vector<std::shared_ptr<Message>> * _buf_read;
	ServiceState _state;
};

// 工作服务
class Service : public noncopyable
{
public:
	static const int32_t kDefaultMaxWaitDestroyTime = 3000;   // 毫秒

	// 初始化（创建服务成功后调用，此时还未开始运行）
	virtual void Init() = 0;

	// 代理服务消息
	virtual void OnProxyServiceMessage(const std::shared_ptr<ProxyServiceMessage> & msg) {}

	// 新连接到来
	virtual void OnNewConnection(const ListenAddress & listen_addr_info, const std::shared_ptr<sframe::TcpSocket> & sock) {}

	// 处理销毁
	virtual void OnDestroy() {}

	// 是否销毁完成
	virtual bool IsDestroyCompleted() const { return true; }

	// 获取该服务的销毁优先级
	// 返回：大于等于0的数字，数字越大越靠后销毁，相同的一起销毁，默认为0
	virtual int32_t GetDestroyPriority() const { return 0; }

	// 处理周期定时器
	virtual void OnCycleTimer() {}

	// 获取服务的循环定时器周期，重写此方法返回大于0的值(ms)设置循环周期
	virtual int32_t GetCyclePeriod() const { return 0; }
	
public:
    Service() : _sid(0), _cur_time(0), _msg_queue(this), _sender_sid(0), _cur_session_key(0), _destroyed(false) {}

    virtual ~Service() {}

	void SetServiceId(int32_t sid)
	{
		_sid = sid;
	}

	int32_t GetServiceId() const
	{
		return _sid;
	}

	// 是否已经被销毁
	bool IsDestroyed() const
	{
		return _destroyed;
	}

    // 压入消息
	void PushMsg(const std::shared_ptr<Message> & msg)
	{
		_msg_queue.Push(msg);
	}

    // 处理
    void Process();

	// 等待销毁完毕
	void WaitDestroyComplete();

	// 获取当前正在处理的服务消息的发送者的ServiceId
	// 只有在服务消息处理函数中，调用此方法有效
	int32_t GetSenderServiceId() const
	{
		return _sender_sid;
	}

	// 获取当前正在处理的服务消息中的session_key
	// 只有在服务消息处理函数中，调用此方法有效
	int64_t GetCurSessionKey() const
	{
		return _cur_session_key;
	}

	// 获取当前时间
	int64_t GetTime() const
	{
		return _cur_time;
	}

	// 注册服务消息处理函数(内部服务消息和网络服务消息都注册)
	template<typename Func_Type, typename Obj_Type>
	void RegistServiceMessageHandler(int msg_id, Func_Type func, Obj_Type * obj)
	{
		_inside_delegate_mgr.Regist(msg_id, func, obj);
		_net_delegate_mgr.Regist(msg_id, func, obj);
	}

	// 仅注册内部服务消息处理函数
	template<typename Func_Type, typename Obj_Type>
	void RegistInsideServiceMessageHandler(int msg_id, Func_Type func, Obj_Type * obj)
	{
		_inside_delegate_mgr.Regist(msg_id, func, obj);
	}

	// 仅注册网络服务消息处理函数
	template<typename Func_Type, typename Obj_Type>
	void RegistNetServiceMessageHandler(int msg_id, Func_Type func, Obj_Type * obj)
	{
		_net_delegate_mgr.Regist(msg_id, func, obj);
	}

	// 注册服务消息处理函数(内部服务消息和网络服务消息都注册)
	template<typename Func_Type, typename Obj_Type>
	void RegistServiceMessageHandler(int msg_id, Func_Type func, const std::function<Obj_Type*(const int64_t &)> & obj_finder)
	{
		_inside_delegate_mgr.Regist(msg_id, func, obj_finder);
		_net_delegate_mgr.Regist(msg_id, func, obj_finder);
	}

	// 仅注册内部服务消息处理函数
	template<typename Func_Type, typename Obj_Type>
	void RegistInsideServiceMessageHandler(int msg_id, Func_Type func, const std::function<Obj_Type*(const int64_t &)> & obj_finder)
	{
		_inside_delegate_mgr.Regist(msg_id, func, obj_finder);
	}

	// 仅注册网络服务消息处理函数
	template<typename Func_Type, typename Obj_Type>
	void RegistNetServiceMessageHandler(int msg_id, Func_Type func, const std::function<Obj_Type*(const int64_t &)> & obj_finder)
	{
		_net_delegate_mgr.Regist(msg_id, func, obj_finder);
	}

	// 发送内部服务消息
	template<typename... T_Args>
	void SendInsideServiceMsg(int32_t dest_sid, int64_t session_key, uint16_t msg_id, T_Args&... args)
	{
		ServiceDispatcher::Instance().SendInsideServiceMsg(_sid, dest_sid, session_key, msg_id, args...);
	}

	// 发送网络服务消息
	template<typename... T_Args>
	void SendNetServiceMsg(int32_t dest_sid, int64_t session_key, uint16_t msg_id, T_Args&... args)
	{
		ServiceDispatcher::Instance().SendNetServiceMsg(_sid, dest_sid, session_key, msg_id, args...);
	}

	// 发送服务消息
	template<typename... T_Args>
	void SendServiceMsg(int32_t dest_sid, int64_t session_key, uint16_t msg_id, T_Args&... args)
	{
		ServiceDispatcher::Instance().SendServiceMsg(_sid, dest_sid, session_key, msg_id, args...);
	}

private:
	// 内部消息委托调用
	void DelegateInsideServiceMsg(const std::shared_ptr<sframe::ServiceMessage> & msg);

	// 网络消息委托调用
	void DelegateNetServiceMsg(const std::shared_ptr<sframe::NetServiceMessage> & msg);

private:
	int32_t _sid;
	int64_t _cur_time;               // 当前时间
	MessageQueue _msg_queue;         // 消息队列
	int32_t _sender_sid;             // 当前正在处理的服务消息的源服务ID
	int64_t _cur_session_key;        // 当前正在处理的服务消息中的会话ID
	bool _destroyed;                 // 是否已被销毁
	DelegateManager<InsideServiceMessageDecoder> _inside_delegate_mgr;
	DelegateManager<NetServiceMessageDecoder> _net_delegate_mgr;
};

}

#endif