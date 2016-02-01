
#ifndef SFRAME_MESSAGE_H
#define SFRAME_MESSAGE_H

#include <inttypes.h>
#include <memory.h>
#include <assert.h>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <tuple>
#include <memory>

namespace sframe{

// 枚举：消息类型
enum MessageType : int32_t
{
	kMsgType_CycleMessage,            // 周期消息
	kMsgType_StartServiceMessage,     // 启动服务消息
	kMsgType_DestroyServiceMessage,   // 销毁服务消息
	kMsgType_ServiceJoinMessage,      // 服务接入消息
	kMsgType_NetServiceMessage,       // 网络服务间消息
	kMsgType_InsideServiceMessage,    // 内部服务间消息
};

// 消息基类
class Message
{
public:
    Message() {}
    virtual ~Message() {}

    // 获取消息类型
    virtual MessageType GetType() const = 0;
};

// 周期消息
class CycleMessage : public Message
{
public:
    CycleMessage(int32_t period) : _period(period) 
    {
        _locked.store(false);
    }

    // 获取消息类型
    MessageType GetType() const override
    {
        return kMsgType_CycleMessage;
    }

    int32_t GetPeriod() const
    {
        return _period;
    }

    bool TryLock()
    {
        bool compare = false;
        return _locked.compare_exchange_strong(compare, true);
    }

    void Unlock()
    {
        _locked.store(false);
    }

private:
    int32_t _period;          // 周期，毫秒
    std::atomic_bool _locked;  // 是否锁定
};

// 服务消息
class ServiceMessage : public Message
{
public:
	ServiceMessage(): src_sid(0), dest_sid(0), msg_id(0) {}

public:
	int32_t src_sid;     // 发送源服务ID
	int32_t dest_sid;    // 目标服务ID
	uint16_t msg_id;     // 消息号
};

// 网络服务消息
class NetServiceMessage : public ServiceMessage
{
public:

	NetServiceMessage(){}

	~NetServiceMessage() {}

	// 获取消息类型
	MessageType GetType() const override
	{
		return kMsgType_NetServiceMessage;
	}

public:
	std::vector<char> data;
};

// 内部服务间消息
template<typename... Data_Type>
class InsideServiceMessage : public ServiceMessage
{
public:
	InsideServiceMessage(Data_Type&... datas) : _data(datas...){}

	// 获取消息类型
	MessageType GetType() const override
	{
		return kMsgType_InsideServiceMessage;
	}

	// 获取数据
	std::tuple<Data_Type...> & GetData()
	{
		return _data;
	}

private:
	std::tuple<Data_Type...> _data;
};

// 启动服务消息
class StartServiceMessage : public Message
{
public:
	// 获取消息类型
	MessageType GetType() const
	{
		return kMsgType_StartServiceMessage;
	}
};

// 销毁服务消息
class DestroyServiceMessage : public Message
{
public:
	// 获取消息类型
	MessageType GetType() const
	{
		return kMsgType_DestroyServiceMessage;
	}
};

// 服务接入消息
class ServiceJoinMessage : public Message
{
public:
	ServiceJoinMessage() {}
	virtual ~ServiceJoinMessage() {}

	// 获取消息类型
	MessageType GetType() const
	{
		return kMsgType_ServiceJoinMessage;
	}

public:
	std::unordered_set<int32_t> service;
	bool is_remote;  // 是否为远程服务
};

}

#endif