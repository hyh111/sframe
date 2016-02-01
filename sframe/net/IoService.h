
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

	virtual Error Init() = 0;

	virtual void RunOnce(int32_t wait_ms, Error & err) = 0;
};

}

#endif