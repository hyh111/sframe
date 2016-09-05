
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <assert.h>
#include "TcpAcceptor_Linux.h"
#include "TcpSocket_Linux.h"
#include "IoService_Linux.h"

using namespace sframe;

// 创建
std::shared_ptr<TcpAcceptor> TcpAcceptor::Create(const std::shared_ptr<IoService> & io_service)
{
	assert(io_service != nullptr);
    std::shared_ptr<TcpAcceptor> obj = std::make_shared<TcpAcceptor_Linux>(io_service);
    return obj;
}


// 开始
Error TcpAcceptor_Linux::Start(const SocketAddr & addr)
{
    if (_monitor == nullptr)
    {
        return ErrorUnknown;
    }

    bool comp = false;
    if (!_runing.compare_exchange_strong(comp, true))
    {
        return ErrorSuccess;
    }

    do
    {
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (_sock < 0)
        {
            break;
        }

		int optval = 1;
		if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		{
			break;
		}

        if (!IoUnit::SetNonBlock(_sock))
        {
            break;
        }

        // 绑定地址
        sockaddr_in bind_addr;
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = addr.GetPort();
        bind_addr.sin_addr.s_addr = addr.GetIp();
        if (bind(_sock, (sockaddr*)&bind_addr, sizeof(bind_addr)) < 0)
        {
            break;
        }

        // 开始监听
        if (listen(_sock, SOMAXCONN) < 0)
        {
            break;
        }

        // 添加EPOLL事件
        IoEvent io_evt = EPOLLIN | EPOLLET;
        if (!((IoService_Linux*)(_io_service.get()))->AddIoEvent(*this, io_evt))
        {
            break;
        }

        return ErrorSuccess;

    }while(false);

    Error err(errno);

    if (_sock >= 0)
    {
        close(_sock);
        _sock = -1;
    }

    _runing.store(false);

    return err;
}

// 停止
bool TcpAcceptor_Linux::Close()
{
    bool comp = true;
    if (!_runing.compare_exchange_strong(comp, false))
    {
        return false;
    }

	_cur_msg.io_unit = shared_from_this();
	((IoService_Linux*)(_io_service.get()))->PostIoMsg(_cur_msg);

	return true;
}

// 事件通知
void TcpAcceptor_Linux::OnEvent(IoEvent io_evt)
{
    if (io_evt & (EPOLLERR | EPOLLHUP))
    {
        int error = 0;
        socklen_t error_len = sizeof(error);
        getsockopt(_sock, SOL_SOCKET, SO_ERROR, &error, &error_len);
        CloseAndNotify(Error(error));
    }
    else if (io_evt & EPOLLIN)
    {
        Accept();
    }
}

// IO消息
void TcpAcceptor_Linux::OnMsg(IoMsg * io_msg)
{
	assert(io_msg == &_cur_msg && _runing.load() == false);

	_cur_msg.io_unit.reset();

	if (io_msg->msg_type == kIoMsgType_Close)
	{
		((IoService_Linux*)(_io_service.get()))->DeleteIoEvent(*this, EPOLLIN | EPOLLET);
		close(_sock);
		_sock = -1;
		if (_monitor)
		{
			_monitor->OnClosed(ErrorSuccess);
		}
	}
}

// 接受连接
void TcpAcceptor_Linux::Accept()
{
    while (true)
    {
        sockaddr_in remote_addr_in;
        socklen_t addr_len = sizeof(remote_addr_in);
        int sock = accept(_sock, (sockaddr*)&remote_addr_in, &addr_len);
        if (sock < 0)
        {
            if (errno != EAGAIN)
            {
                CloseAndNotify(Error(errno));
            }
            return;
        }

		// 设置套接字非阻塞
		if (!IoUnit::SetNonBlock(sock))
		{
			Error err(errno);
			if (_monitor)
			{
				_monitor->OnAccept(nullptr, err);
			}
		}
		else
		{
			// 获取绑定的本地地址
			sockaddr_in local_addr_in{};
			getsockname(sock, (sockaddr*)&local_addr_in, &addr_len);

			SocketAddr local_addr(local_addr_in.sin_addr.s_addr, local_addr_in.sin_port);
			SocketAddr remote_addr(remote_addr_in.sin_addr.s_addr, remote_addr_in.sin_port);

			// 创建
			std::shared_ptr<TcpSocket> sock_obj = TcpSocket_Linux::Create(_io_service, sock, &local_addr, &remote_addr);
			if (_monitor)
			{
				_monitor->OnAccept(sock_obj, ErrorSuccess);
			}
		}
    }
}

// 关闭并通知
void TcpAcceptor_Linux::CloseAndNotify(Error err)
{
    bool comp = true;
    if (!_runing.compare_exchange_strong(comp, false))
    {
        return;
    }

	((IoService_Linux*)(_io_service.get()))->DeleteIoEvent(*this, EPOLLIN | EPOLLET);
    close(_sock);
    _sock = -1;
	if (_monitor)
	{
		_monitor->OnClosed(err);
	}
}