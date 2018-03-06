
#ifndef SFRAME_IO_SERVICE_LINUX_H
#define SFRAME_IO_SERVICE_LINUX_H

#include <atomic>
#include <vector>
#include "../IoService.h"
#include "IoUnit.h"
#include "../../util/Lock.h"

namespace sframe {

// LinuxÏÂµÄIo·þÎñ(²ÉÓÃEPOLLÊµÏÖ)
class IoService_Linux : public IoService
{
public:
	// epollµÈ´ý×î´óÊÂ¼þÊýÁ¿
	static const int kMaxEpollEventsNumber = 1024;
	// IOÏûÏ¢»º³åÇø³¤¶È
	static const int kMaxIoMsgBufferSize = 65536;

public:
	IoService_Linux();

	virtual ~IoService_Linux();

	Error Init() override;

	void RunOnce(int32_t wait_ms, Error & err) override;

	void Close() override;

	// Ìí¼Ó¼àÌýÊÂ¼þ
	bool AddIoEvent(const IoUnit & iounit, const IoEvent ioevt);

	// ÐÞ¸Ä¼àÌýÊÂ¼þ
	bool ModifyIoEvent(const IoUnit & iounit, const IoEvent ioevt);

	// É¾³ý¼àÌýÊÂ¼þ
	bool DeleteIoEvent(const IoUnit & iounit, const IoEvent ioevt);

	// Í¶µÝÏûÏ¢
	void PostIoMsg(const IoMsg & io_msg);

private:
	int _epoll_fd;
	int _msg_evt_fd;               // ÓÃÓÚÊµÏÖIOÏûÏ¢µÄ·¢ËÍÓë´¦Àí
	std::vector<IoMsg*> _msgs;     // IOÏûÏ¢ÁÐ±í
	sframe::Lock _msgs_lock;  // ÏûÏ¢ÁÐ±íËø
};

}

#endif
