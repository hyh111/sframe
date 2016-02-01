
#include <assert.h>
#include "IoService_Win.h"
#include "Initialize.h"

using namespace sframe;

std::shared_ptr<IoService> IoService::Create()
{
	std::shared_ptr<IoService> ioservice = std::make_shared<IoService_Win>();
	return ioservice;
}


IoService_Win::IoService_Win()
{
    WinSockInitial * initial = &g_win_sock_initial;

	_busy.store(false);
	_iocp = nullptr;
}

IoService_Win::~IoService_Win()
{
	if (_iocp != nullptr)
	{
		CloseHandle(_iocp);
	}
}

Error IoService_Win::Init()
{
	// 创建完成端口
	_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (_iocp == nullptr)
	{
		Error err = GetLastError();
		return err;
	}

	return ErrorSuccess;
}

void IoService_Win::RunOnce(int32_t wait_ms, Error & err)
{
	err = ErrorSuccess;

	bool cmp_temp = false;
	if (!_busy.compare_exchange_strong(cmp_temp, true))
	{
		return;
	}

	ULONG_PTR complete_key = 0;
	LPOVERLAPPED obj = nullptr;
	DWORD trans_bytes = 0;
	DWORD err_code = ERROR_SUCCESS;
	BOOL ret = GetQueuedCompletionStatus(_iocp, &trans_bytes, &complete_key, &obj, (DWORD)wait_ms);
	if (!ret)
	{
		err_code = GetLastError();

		if (obj == nullptr)
		{
			if (err_code != WAIT_TIMEOUT)
			{
				err = err_code;
			}
			else
			{
				_busy.store(false);
			}

			return;
		}
	}

	assert(obj);

	if (complete_key == 0)
	{
		IoMsg * io_msg = (IoMsg*)obj;
		std::shared_ptr<IoUnit> io_obj = io_msg->io_unit;
		if (io_obj)
		{
			io_obj->OnMsg(io_msg);
		}
	}
	else
	{
		IoEvent * io_evt = (IoEvent*)obj;
		std::shared_ptr<IoUnit> io_obj = io_evt->io_unit;
		if (io_obj)
		{
			io_evt->err = err_code;
			io_evt->data_len = trans_bytes;
			io_obj->OnEvent(io_evt);
		}
	}

	_busy.store(false);
}

// 注册Socket
bool IoService_Win::RegistSocket(const IoUnit & sock)
{
	if (!this->_iocp)
	{
		return false;
	}

    SOCKET sd = sock.GetSocket();
    // 关联套接字到完成端口
    if (CreateIoCompletionPort((HANDLE)sd, this->_iocp, (ULONG_PTR)&sock, 0) == NULL)
    {
        return false;
    }

    return true;
}

// 投递消息
void IoService_Win::PostIoMsg(const IoMsg & io_msg)
{
	BOOL ret = PostQueuedCompletionStatus(_iocp, 0, (ULONG_PTR)0, (LPOVERLAPPED)&io_msg);
	DWORD err_code = GetLastError();
	assert(ret);
}