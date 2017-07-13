
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
#include "../util/TupleHelper.h"
#include "../util/Serialization.h"

namespace sframe{

// 枚举：消息类型
enum MessageType : int32_t
{
	kMsgType_CycleMessage,            // 周期消息
	kMsgType_DestroyServiceMessage,   // 销毁服务消息
	kMsgType_NewConnectionMessage,    // 新连接消息
	kMsgType_NetServiceMessage,       // 网络服务间消息
	kMsgType_InsideServiceMessage,    // 内部服务间消息
	kMsgType_ProxyServiceMessage,     // 代理服务消息
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
		_locked.clear(std::memory_order_relaxed);
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

	void SetPeriod(int32_t period)
	{
		_period = period;
	}

    bool TryLock()
    {
		return (!_locked.test_and_set(std::memory_order_acquire));
    }

    void Unlock()
    {
		_locked.clear(std::memory_order_relaxed);
    }

private:
    int32_t _period;           // 周期，毫秒
    std::atomic_flag _locked;  // 是否锁定
};

// 服务消息
class ServiceMessage : public Message
{
public:
	ServiceMessage(): src_sid(0), dest_sid(0), msg_id(0) {}

public:
	int32_t src_sid;     // 发送源服务ID
	int32_t dest_sid;    // 目标服务ID
	int64_t session_key; // 会话key
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
	InsideServiceMessage(const Data_Type&... datas) : _data(datas...){}

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

// 代理服务消息
class ProxyServiceMessage : public ServiceMessage
{
public:
	ProxyServiceMessage() {}

	// 获取消息类型
	MessageType GetType() const override
	{
		return kMsgType_ProxyServiceMessage;
	}

	// 序列化
	virtual bool Serialize(char * buf, int32_t * len) = 0;
};

// 具体的代理服务消息
template<typename... Data_Type>
class ProxyServiceMessageT : public ProxyServiceMessage
{
public:
	ProxyServiceMessageT(Data_Type&... datas) : _data(datas...) {}

	// 序列化
	bool Serialize(char * buf, int32_t * len) override
	{
		if (buf == nullptr || len == nullptr || (*len) < 0)
		{
			return false;
		}

		_buf = buf;
		_len = len;

		return UnfoldTuple(this, _data);
	}

	template<typename... Args>
	bool DoUnfoldTuple(Args&&... args)
	{
		assert(_buf && _len && (*_len) >= 0);
		uint16_t msg_size = 0;
		StreamWriter writer(_buf + sizeof(msg_size), (uint32_t)(*_len) - sizeof(msg_size));
		if (!AutoEncode(writer, src_sid, dest_sid, session_key, msg_id, args...))
		{
			return false;
		}

		msg_size = (uint16_t)writer.GetStreamLength();
		msg_size = HTON_16(msg_size);
		memcpy(_buf, (void*)&msg_size, sizeof(msg_size));

		(*_len) = (int32_t)writer.GetStreamLength() + (int32_t)sizeof(msg_size);

		return true;
	}

private:
	std::tuple<Data_Type...> _data;
	char * _buf;
	int32_t * _len;
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

// 监听地址
struct ListenAddress
{
	std::string desc_name;
	std::string ip;
	uint16_t port;
};

class TcpSocket;

// 接收到新连接消息
class NewConnectionMessage : public Message
{
public:
	NewConnectionMessage(const std::shared_ptr<TcpSocket> & sock, const ListenAddress & listen_addr)
		: _sock(sock), _listen_addr(listen_addr) {}
	virtual ~NewConnectionMessage() {}

	// 获取消息类型
	MessageType GetType() const
	{
		return kMsgType_NewConnectionMessage;
	}

	const std::shared_ptr<TcpSocket> & GetSocket() const
	{
		return _sock;
	}

	const ListenAddress & GetListenAddress() const
	{
		return _listen_addr;
	}

private:
	std::shared_ptr<TcpSocket> _sock;
	ListenAddress _listen_addr;
};

}

#endif