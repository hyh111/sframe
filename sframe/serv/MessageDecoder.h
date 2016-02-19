
#ifndef SFRAME_MESSAGE_DECODER_H
#define SFRAME_MESSAGE_DECODER_H

#include "Message.h"
#include "../util/Serialization.h"
#include "../util/TupleHelper.h"

namespace sframe {

// 内部服务消息解码器
class InsideServiceMessageDecoder
{
public:
	InsideServiceMessageDecoder(ServiceMessage * msg) : _msg(msg)
	{
		assert(_msg);
	}

	template<typename... Args>
	bool Decode(std::tuple<Args...> ** p_args_tuple, std::tuple<Args...> & args_tuple)
	{
		MessageType msg_type = _msg->GetType();
		assert(msg_type == kMsgType_InsideServiceMessage);

		InsideServiceMessage<Args...> * msg = dynamic_cast<InsideServiceMessage<Args...>*>(_msg);
		if (msg == nullptr)
		{
			return false;
		}

		(*p_args_tuple) = &msg->GetData();

		return true;
	}

private:
	ServiceMessage * _msg;
};

// 网络服务消息解码器
class NetServiceMessageDecoder
{
public:
	NetServiceMessageDecoder(ServiceMessage * msg) : _msg(msg)
	{
		assert(_msg);
	}

	template<typename... Args>
	bool Decode(std::tuple<Args...> ** p_args_tuple, std::tuple<Args...> & args_tuple)
	{
		assert(_msg->GetType() == kMsgType_NetServiceMessage);
		
		return UnfoldTuple(this, args_tuple);
	}

	template<typename... Args>
	bool DoUnfoldTuple(Args&&... args)
	{
		NetServiceMessage * msg = dynamic_cast<NetServiceMessage*>(_msg);
		if (msg == nullptr)
		{
			return false;
		}

		int32_t src_sid = 0;
		int32_t dest_sid = 0;
		uint16_t msg_id = 0;
		StreamReader stream_reader(&(msg->data)[0], (uint32_t)msg->data.size());
		return AutoDecode(stream_reader, src_sid, dest_sid, msg_id, args...);
	}

private:
	ServiceMessage * _msg;
};

}

#endif
