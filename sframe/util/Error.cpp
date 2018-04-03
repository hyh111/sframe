
#ifndef __GNUC__
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#endif

#include "Error.h"

using namespace sframe;

#ifndef __GNUC__

const ErrorCode Error::kErrorCode_Succ = ERROR_SUCCESS;

const ErrorCode Error::kErrorCode_Fail = 0xffffffff;

#else

const ErrorCode Error::kErrorCode_Succ = 0;

const ErrorCode Error::kErrorCode_Fail = -1;

#endif




ErrorMessage::ErrorMessage(Error err)
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