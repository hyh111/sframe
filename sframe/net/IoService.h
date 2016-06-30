
#ifndef SFRAME_IO_SERVICE_H
#define SFRAME_IO_SERVICE_H

#include <memory>
#include "Error.h"

namespace sframe{

// Io服务
class IoService
{
public:
	static std::shared_ptr<IoService> Create();

public:

	IoService() : _open(false) {}

	virtual Error Init() = 0;

	// 线程不安全，需保证同时只有一个线程在执行此函数
	virtual void RunOnce(int32_t wait_ms, Error & err) = 0;

	virtual void Close() = 0;

	bool IsOpen()
	{
		return _open;
	}

protected:
	bool _open;
};

}

#endif