
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <assert.h>
#include "TcpSocket_Linux.h"
#include "IoService_Linux.h"

using namespace sframe;

// 创建Socket对象
std::shared_ptr<TcpSocket> TcpSocket::Create(const std::shared_ptr<IoService> & ioservice)
{
	assert(ioservice != nullptr);
    std::shared_ptr<TcpSocket> obj = std::make_shared<TcpSocket_Linux>(ioservice);
    return obj;
}

// 创建Socket对象
std::shared_ptr<TcpSocket_Linux> TcpSocket_Linux::Create(const std::shared_ptr<IoService> & ioservice, int sock,
	const SocketAddr * local_addr, const SocketAddr * remote_addr)
{
	assert(ioservice != nullptr);
	std::shared_ptr<TcpSocket_Linux> obj = std::make_shared<TcpSocket_Linux>(ioservice);
	obj->_sock = sock;
	if (local_addr)
	{
		obj->_local_addr = *local_addr;
	}
	if (remote_addr)
	{
		obj->_remote_addr = *remote_addr;
	}

	obj->_state.store((int32_t)kState_Opened);

	return obj;
}

TcpSocket_Linux::TcpSocket_Linux(const std::shared_ptr<IoService> & io_service)
	: IoUnit(io_service), _add_evt(false), _cur_msg(kIoMsgType_SendData), _cur_events(EPOLLET),
	_recv_len(0), _last_error(0), _tcp_nodelay(false)
{}

// 连接
void TcpSocket_Linux::Connect(const SocketAddr & remote)
{
    int32_t comp = TcpSocket::kState_Initial;
    if (!_state.compare_exchange_strong(comp, TcpSocket::kState_Connecting))
    {
        return;
    }

	_remote_addr = remote;
	_cur_msg.msg_type = kIoMsgType_Connect;
	_cur_msg.io_unit = shared_from_this();
	((IoService_Linux*)(_io_service.get()))->PostIoMsg(_cur_msg);
}

// 发送数据
void TcpSocket_Linux::Send(const char * data, int32_t len)
{
	auto cur_state = GetState();
    if (cur_state != kState_Opened)
    {
        return;
    }

	bool send_now = false;
	_send_buf.Push(data, len, send_now);

    if (send_now)
    {
		// 向IO服务投递发送数据的消息
		_cur_msg.msg_type = kIoMsgType_SendData;
		_cur_msg.io_unit = shared_from_this();
		((IoService_Linux*)(_io_service.get()))->PostIoMsg(_cur_msg);
    }
}

// 开始接收数据(设置监听器后调用一次)
void TcpSocket_Linux::StartRecv()
{
    if (GetState() != kState_Opened || _recv_len >= kRecvBufSize)
    {
        return;
    }

    // 等待可读
    if (!ModifyEpollEvent(EPOLLIN))
    {
		_last_error = errno;

		int32_t comp = TcpSocket::kState_Opened;
		if (!_state.compare_exchange_strong(comp, (int32_t)TcpSocket::kState_Closed))
		{
			return;
		}

		_cur_msg.msg_type = kIoMsgType_NotifyError;
		_cur_msg.io_unit = shared_from_this();
		((IoService_Linux*)(_io_service.get()))->PostIoMsg(_cur_msg);
    }
}

// 关闭
bool TcpSocket_Linux::Close()
{
	int32_t cmp_conn = TcpSocket::kState_Connecting;
	int32_t cmp_open = TcpSocket::kState_Opened;
    if (_state.compare_exchange_strong(cmp_conn, (int32_t)TcpSocket::kState_Closed) ||
		_state.compare_exchange_strong(cmp_open, (int32_t)TcpSocket::kState_Closed))
    {
		_cur_msg.msg_type = kIoMsgType_Close;
		_cur_msg.io_unit = shared_from_this();
		((IoService_Linux*)(_io_service.get()))->PostIoMsg(_cur_msg);
		return true;
    }

	return false;
}

// 设置TCP_NODELAY
Error TcpSocket_Linux::SetTcpNodelay(bool on)
{
	if (on == _tcp_nodelay)
	{
		return ErrorSuccess;
	}

	_tcp_nodelay = on;
	if (_sock != -1)
	{
		int nodelayopt = on ? 1 : 0;
		if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (const void *)&nodelayopt, sizeof(nodelayopt)) == -1)
		{
			Error err(errno);
			return err;
		}
	}

	return ErrorSuccess;
}

void TcpSocket_Linux::OnEvent(IoEvent io_evt)
{
	uint32_t evts = io_evt;

	int error = 0;
	socklen_t error_len = sizeof(error);
	State s = GetState();

	if (s == kState_Connecting)
	{
		getsockopt(_sock, SOL_SOCKET, SO_ERROR, &error, &error_len);
		if (io_evt == EPOLLOUT && error == 0)
		{
			_state.store(kState_Opened);
			if (_monitor)
			{
				this->_monitor->OnConnected(ErrorSuccess);
			}
		}
		else
		{
			((IoService_Linux*)(_io_service.get()))->DeleteIoEvent(*this, _cur_events);
			close(_sock);
			_sock = -1;
			_state.store(kState_Closed);
			if (_monitor)
			{
				this->_monitor->OnConnected(Error(error));
			}
		}
	}
	else if (s == kState_Opened)
	{
		if (io_evt & (EPOLLERR | EPOLLHUP))
		{
			getsockopt(_sock, SOL_SOCKET, SO_ERROR, &error, &error_len);
			CloseAndNotify(Error(error));
		}
		else
		{
			if (io_evt & EPOLLOUT)
			{
				if (!SendData())
				{
					return;
				}
			}

			if (io_evt & EPOLLIN)
			{
				if (!RecvData())
				{
					return;
				}
			}
		}
	}

}

void TcpSocket_Linux::OnMsg(IoMsg * io_msg)
{
    assert(io_msg == &_cur_msg);
	TcpSocket::State s = GetState();
	_cur_msg.io_unit.reset();

	if (_cur_msg.msg_type == kIoMsgType_Connect)
	{
		if (s != TcpSocket::kState_Connecting)
		{
			return;
		}

		Connect();
	}
	else if (_cur_msg.msg_type == kIoMsgType_SendData)
	{
		if (s != TcpSocket::kState_Opened)
		{
			return;
		}

		SendData();
	}
	else if (_cur_msg.msg_type == kIoMsgType_Close)
	{
		assert(s == TcpSocket::kState_Closed);

		((IoService_Linux*)(_io_service.get()))->DeleteIoEvent(*this, _cur_events);
		shutdown(_sock, SHUT_RDWR);
		close(_sock);
		_sock = -1;
		if (_monitor)
		{
			_monitor->OnClosed(true, ErrorSuccess);
		}
	}
	else if (_cur_msg.msg_type == kIoMsgType_NotifyError)
	{
		assert(s == TcpSocket::kState_Closed);
		shutdown(_sock, SHUT_RDWR);
		close(_sock);
		_sock = -1;
		_monitor->OnClosed(false, _last_error);
	}
}


// 修改Epoll的等待事件
bool TcpSocket_Linux::ModifyEpollEvent(uint32_t evt)
{
    if (_cur_events & evt)
    {
        return true;
    }

    bool ret = false;
    uint32_t event = _cur_events | evt;

    if (!_add_evt)
    {
        ret = ((IoService_Linux*)(_io_service.get()))->AddIoEvent(*this, event);
        if (ret)
        {
            _add_evt = true;
        }
    }
    else
    {
        ret = ((IoService_Linux*)(_io_service.get()))->ModifyIoEvent(*this, event);
    }

    if (ret)
    {
        _cur_events = event;
    }

    return ret;
}

// 连接
void TcpSocket_Linux::Connect()
{
	do
	{
		// 新建套接字
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (_sock < 0)
		{
			break;
		}

		// 设置套接字非阻塞
		if (!IoUnit::SetNonBlock(_sock))
		{
			break;
		}

		if (_tcp_nodelay)
		{
			// 设置NODELAY
			int nodelayopt = 1;
			if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (const void *)&nodelayopt, sizeof(nodelayopt)) == -1)
			{
				break;
			}
		}

		sockaddr_in remote_addr;
		remote_addr.sin_family = AF_INET;
		remote_addr.sin_port = _remote_addr.GetPort();
		remote_addr.sin_addr.s_addr = _remote_addr.GetIp();
		// 连接
		if (connect(_sock, (const sockaddr *)&remote_addr, sizeof(remote_addr)) == 0)
		{
			_state.store(kState_Opened);
			if (_monitor)
			{
				_monitor->OnConnected(ErrorSuccess);
			}
			return;
		}

		if (errno != EINPROGRESS)
		{
			break;
		}

		// 关联到epoll等待连接完成
		if (!this->ModifyEpollEvent(EPOLLOUT))
		{
			break;
		}

		return;

	} while (false);

	Error err(errno);

	if (_sock >= 0)
	{
		close(_sock);
		_sock = -1;
	}

	_state.store(kState_Closed);

	if (_monitor)
	{
		_monitor->OnConnected(err);
	}
}

// 发送数据
bool TcpSocket_Linux::SendData()
{
    int32_t peek_len = 0;
    char * buf = _send_buf.Peek(peek_len);
    while (peek_len > 0)
    {
        int32_t ret = (int32_t)write(_sock, buf, peek_len);
        if (ret < 0)
        {
            if (errno != EAGAIN)
            {
                CloseAndNotify(Error(errno));
                return false;
            }

            ret = 0;
        }

        assert(peek_len >= ret);
		_send_buf.Free(ret);

        if (peek_len > ret)
        {
            // 等待可写
            if (!ModifyEpollEvent(EPOLLOUT))
            {
                CloseAndNotify(Error(errno));
                return false;
            }
            return true;
        }

        // 再次读取待发送数据
        buf = _send_buf.Peek(peek_len);
    }

    return true;
}

// 接收数据
bool TcpSocket_Linux::RecvData()
{
    while(true)
    {
        int32_t empty_len = kRecvBufSize - _recv_len;
        assert(empty_len > 0);
        int len = read(_sock, _recv_buf + _recv_len, empty_len);
        assert(len <= empty_len);
        if (len == 0)
        {
            CloseAndNotify(ErrorSuccess);
            return false;
        }
        else if (len < 0)
        {
            if (errno != EAGAIN)
            {
                CloseAndNotify(Error(errno));
                return false;
            }
            break;
        }

        _recv_len += len;

        // 通知
		int32_t surplus = 0;
		
		if (_monitor)
		{
			surplus = _monitor->OnReceived(_recv_buf, _recv_len);
		}
		
        surplus = surplus > _recv_len ? _recv_len : surplus;
		
        if (surplus < 0)
        {
            CloseAndNotify(ErrorSuccess);
            return false;
        }
        else if (surplus >= kRecvBufSize)
        {
			// 或者剩余数据已占满缓冲区，此时直接关闭连接，以免造成数据混乱
			CloseAndNotify(ErrorSuccess);
			return false;
        }
        else
        {
            memcpy(this->_recv_buf, this->_recv_buf + (_recv_len - surplus), surplus);
            _recv_len = surplus;
        }

        if (len < empty_len)
        {
            break;
        }
    }

    return true;
}

// 关闭并通知
void TcpSocket_Linux::CloseAndNotify(Error err)
{
    int32_t comp = TcpSocket::kState_Opened;
    if (!_state.compare_exchange_strong(comp, (int32_t)TcpSocket::kState_Closed))
    {
        return;
    }

	((IoService_Linux*)(_io_service.get()))->DeleteIoEvent(*this, _cur_events);
    shutdown(_sock, SHUT_RDWR);
    close(_sock);
    _sock = -1;
	if (_monitor)
	{
		_monitor->OnClosed(false, err);
	}
}