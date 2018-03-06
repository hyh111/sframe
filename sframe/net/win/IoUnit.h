
#ifndef SFRAME_IO_UNIT_H
#define SFRAME_IO_UNIT_H

#include <inttypes.h>
#include <memory>
#include <WinSock2.h>
#include "../Error.h"

namespace sframe {

// IOÊÂ¼þÀàÐÍ
enum IoEventType :int32_t
{
	kIoEvent_ConnectCompleted,      // Á¬½Ó
	kIoEvent_SendCompleted,         // ·¢ËÍ
	kIoEvent_RecvCompleted,         // ½ÓÊÕÊý¾Ý
	kIoEvent_AcceptCompleted,       // ½ÓÊÜÁ¬½Ó
};

class IoUnit;

// IOÊÂ¼þ
struct IoEvent
{
	IoEvent(IoEventType t)
		: evt_type(t), err(kErrorCode_Success)
	{
		memset(&ol, 0, sizeof(ol));
	}

	OVERLAPPED ol;
	const IoEventType evt_type;   // ÊÂ¼þÀàÐÍ
	std::shared_ptr<IoUnit> io_unit; // ÓÃÓÚ¹ØÁªIoUnit¶ÔÏó£¬IO·þÎñÓÃ´Ë½øÐÐÍê³ÉÊÂ¼þÍ¨Öª£¬Í¬Ê±±£Ö¤ÔÚÒ»´Î²Ù×÷Íê³ÉÖ®Ç°¶ÔÏó²»»á±»Îö¹¹µô
	Error err;                    // ´íÎóÂë
	int32_t data_len;             // Êý¾Ý³¤¶È
};

// IOÏûÏ¢ÀàÐÍ
enum IoMsgType : int32_t
{
	kIoMsgType_Close,        // ¹Ø±Õ
	kIoMsgType_NotifyError,  // ´íÎóÍ¨Öª
};

// IOÏûÏ¢
struct IoMsg
{
	IoMsg(IoMsgType t) : msg_type(t) {}

	IoMsgType msg_type;
	std::shared_ptr<IoUnit> io_unit;
};

class IoService;

// Ioµ¥Ôª
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