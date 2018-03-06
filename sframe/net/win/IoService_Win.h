
#ifndef SFRAME_IO_SERVICE_WIN_H
#define SFRAME_IO_SERVICE_WIN_H

#include <atomic>
#include "IoUnit.h"
#include "../IoService.h"

namespace sframe {

// windowsµÄIo·þÎñ(Íê³É¶Ë¿ÚÊµÏÖ)
class IoService_Win : public IoService
{
public:
	IoService_Win();

	virtual ~IoService_Win();

	Error Init() override;

	void RunOnce(int32_t wait_ms, Error & err) override;

	void Close() override;

	// ×¢²áSocket
	bool RegistSocket(const IoUnit & io_unit);

	// Í¶µÝÏûÏ¢
	void PostIoMsg(const IoMsg & io_msg);

private:
	HANDLE _iocp;
};

}

#endif