
#ifndef SFRAME_IO_SERVICE_WIN_H
#define SFRAME_IO_SERVICE_WIN_H

#include <atomic>
#include "IoUnit.h"
#include "../IoService.h"

namespace sframe {

// windows的Io服务(完成端口实现)
class IoService_Win : public IoService
{
public:
	IoService_Win();

	virtual ~IoService_Win();

	Error Init() override;

	void RunOnce(int32_t wait_ms, Error & err) override;

	void Close() override;

	// 注册Socket
	bool RegistSocket(const IoUnit & io_unit);

	// 投递消息
	void PostIoMsg(const IoMsg & io_msg);

private:
	HANDLE _iocp;
};

}

#endif