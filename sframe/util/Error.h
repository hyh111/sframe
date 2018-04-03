
#ifndef SFRAME_ERROR_H
#define SFRAME_ERROR_H

#include <inttypes.h>

namespace sframe{

#ifndef __GNUC__

typedef uint32_t ErrorCode;

#else

typedef int ErrorCode;

#endif

// 错误类
class Error
{
public:

	static const ErrorCode kErrorCode_Succ;

	static const ErrorCode kErrorCode_Fail;


    Error(ErrorCode err_code) : _err_code(err_code) {}

	Error() {}

    ErrorCode Code() const
    {
        return _err_code;
    }

    operator bool() const
    {
        return _err_code != kErrorCode_Succ;
    }

private:
    ErrorCode _err_code;
};

#define ErrorSuccess (Error(Error::kErrorCode_Succ))

#define ErrorUnknown (Error(Error::kErrorCode_Fail))

// 错误描述
class ErrorMessage
{
public:

    static const int kMessageBuffSize = 128;

	ErrorMessage(Error err);

    const char * Message() const
    {
        return p;
    }

private:
    char _err_msg[kMessageBuffSize];
	char * p;
};

}

#endif