
#ifndef SFRAME_IO_UNIT_H
#define SFRAME_IO_UNIT_H

#include <inttypes.h>
#include <memory>
#include <unistd.h>
#include <fcntl.h>

namespace sframe {

class IoUnit;

typedef uint32_t IoEvent;

// IO消息类型
enum IoMsgType : int32_t
{
	kIoMsgType_Connect,      // 连接
	kIoMsgType_SendData,     // 发送数据
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
	// 设置文件描述符为非阻塞
	static bool SetNonBlock(int fd)
	{
		int old = fcntl(fd, F_GETFL);
		if (old < 0)
		{
			return false;
		}

		old |= O_NONBLOCK;
		if (fcntl(fd, F_SETFL, old) < 0)
		{
			return false;
		}

		return true;
	}

public:
	IoUnit(const std::shared_ptr<IoService> & io_service) : _sock(-1), _io_service(io_service) {}

	virtual ~IoUnit()
	{
		if (_sock != -1)
		{
			close(_sock);
		}
	}

	virtual void OnEvent(IoEvent io_evt) = 0;

	virtual void OnMsg(IoMsg * io_msg) = 0;

	int GetSocket() const
	{
		return _sock;
	}

protected:
	int _sock;
	std::shared_ptr<IoService> _io_service;
};

}

#endif