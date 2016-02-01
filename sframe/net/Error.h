
#ifndef SFRAME_ERROR_H
#define SFRAME_ERROR_H

#ifndef __GNUC__
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#endif

namespace sframe{

#ifndef __GNUC__
typedef DWORD ErrorCode;
static const ErrorCode kErrorCode_Success = ERROR_SUCCESS;
#else
typedef int ErrorCode;
static const ErrorCode kErrorCode_Success = 0;
#endif

// 错误类
class Error
{
public:
    Error(ErrorCode err_code) : _err_code(err_code) {}
	Error() {}

    ErrorCode Code() const
    {
        return _err_code;
    }

    operator bool() const
    {
        return _err_code != kErrorCode_Success;
    }

private:
    ErrorCode _err_code;
};

static const Error ErrorSuccess = kErrorCode_Success;

static const Error ErrorUnknown = (ErrorCode)-1;

// 错误描述
class ErrorMessage
{
    static const int kMessageBuffSize = 128;
public:
    ErrorMessage(Error err)
    {
        memset(_err_msg, 0, sizeof(_err_msg));

#ifndef __GNUC__
        // 获取描述
        DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err.Code(), 0, _err_msg, kMessageBuffSize, NULL);
        // 去掉尾部的\r\n
        while (len > 0)
        {
            char c = _err_msg[len - 1];
            if (c != '\r' && c != '\n')
            {
                break;
            }
            _err_msg[len - 1] = 0;
            len--;
        }

		p = _err_msg;
#else
# if _GNU_SOURCE || (_POSIX_C_SOURCE < 200112L && _XOPEN_SOURCE < 600)
        p = strerror_r(err.Code(), _err_msg, kMessageBuffSize);
# else
		strerror_r(err.Code(), _err_msg, kMessageBuffSize);
		p = _err_msg;
#endif
#endif
    }

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