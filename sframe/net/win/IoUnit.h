
#ifndef SFRAME_IO_UNIT_H
#define SFRAME_IO_UNIT_H

#include <inttypes.h>
#include <memory>
#include <WinSock2.h>
#include "../../util/Error.h"

namespace sframe {

// IO事件类型
enum IoEventType :int32_t
{
	kIoEvent_ConnectCompleted,      // 连接
	kIoEvent_SendCompleted,         // 发送
	kIoEvent_RecvCompleted,         // 接收数据
	kIoEvent_AcceptCompleted,       // 接受连接
};

class IoUnit;

// IO事件
struct IoEvent
{
	IoEvent(IoEventType t)
		: evt_type(t), err(Error::kErrorCode_Succ)
	{
		memset(&ol, 0, sizeof(ol));
	}

	OVERLAPPED ol;
	const IoEventType evt_type;   // 事件类型
	std::shared_ptr<IoUnit> io_unit; // 用于关联IoUnit对象，IO服务用此进行完成事件通知，同时保证在一次操作完成之前对象不会被析构掉
	Error err;                    // 错误码
	int32_t data_len;             // 数据长度
};

// IO消息类型
enum IoMsgType : int32_t
{
	kIoMsgType_Close,        // 关闭
	kIoMsgType_NotifyError,  // 错误通知
};

// IO消息
struct IoMsg
{
	IoMsg(IoMsgType t) : msg_type(t) {}

	IoMsgType msg_type;
	std::shared_ptr<IoUnit> io_unit;
};

class IoService;

// Io单元
class IoUnit
{
public:
	IoUnit(const std::shared_ptr<IoService> & io_service) : _sock(INVALID_SOCKET), _io_service(io_service) {}

	virtual ~IoUnit()
	{
		if (_sock != INVALID_SOCKET)
		{
			closesocket(_sock);
		}
	}

	virtual void OnEvent(IoEvent * io_evt) = 0;

	virtual void OnMsg(IoMsg * io_msg) = 0;

	SOCKET GetSocket() const
	{
		return _sock;
	}

protected:
	SOCKET _sock;
	std::shared_ptr<IoService> _io_service;
};

}

#endif