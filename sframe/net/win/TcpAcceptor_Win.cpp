
#include <assert.h>
#include "TcpAcceptor_Win.h"
#include "TcpSocket_Win.h"
#include "IoService_Win.h"

using namespace sframe;

// 创建
std::shared_ptr<TcpAcceptor> TcpAcceptor::Create(const std::shared_ptr<IoService> & ioservice)
{
	assert(ioservice);
    std::shared_ptr<TcpAcceptor> obj = std::make_shared<TcpAcceptor_Win>(ioservice);
    return obj;
}


TcpAcceptor_Win::TcpAcceptor_Win(const std::shared_ptr<IoService> & ioservice)
    : IoUnit(ioservice), _evt_accept(kIoEvent_AcceptCompleted),
    _lpfn_acceptex(nullptr), _lpfn_get_acceptex_sockaddrs(nullptr),
    _accept_sock(INVALID_SOCKET), _cur_msg(kIoMsgType_Close)
{
    _running.store(false);
}


// 开始
Error TcpAcceptor_Win::Start(const SocketAddr & listen_addr)
{
    if (_monitor == nullptr)
    {
        return ErrorUnknown;
    }

    bool temp = false;
    if (!_running.compare_exchange_weak(temp, true))
    {
        return ErrorUnknown;
    }

    // 创建监听套接字
    _sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (_sock == INVALID_SOCKET)
    {
        _running.store(false);
        return Error(GetLastError());
    }

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = listen_addr.GetPort();
    addr.sin_addr.S_un.S_addr = listen_addr.GetIp();
    if (bind(_sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR     // 绑定
        || !((IoService_Win*)(_io_service.get()))->RegistSocket(*this)                     // 注册到IO服务
        || listen(_sock, SOMAXCONN) == SOCKET_ERROR                    // 开始监听
        )
    {
		Error err(WSAGetLastError());
        closesocket(_sock);
        _sock = INVALID_SOCKET;
        _running.store(false);
        return err;
    }

    // 加载AcceptEx函数
    if (this->_lpfn_acceptex == nullptr)
    {
        GUID guid_accept_ex = WSAID_ACCEPTEX;
        DWORD recv_bytes;
        if (WSAIoctl(this->_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_accept_ex, sizeof(guid_accept_ex),
            &this->_lpfn_acceptex, sizeof(this->_lpfn_acceptex), &recv_bytes, nullptr, nullptr) == INVALID_SOCKET)
        {
			Error err(WSAGetLastError());
            closesocket(_sock);
            _sock = INVALID_SOCKET;
            _running.store(false);
            return err;
        }
    }

    Error err = Accept();
    if (err)
    {
        closesocket(_sock);
        _sock = INVALID_SOCKET;
        this->_lpfn_acceptex = nullptr;
        _running.store(false);
        return err;
    }

    return ErrorSuccess;
}

// 停止
bool TcpAcceptor_Win::Close()
{
    bool temp = true;
    if (!_running.compare_exchange_weak(temp, false))
    {
        return false;
    }

	_cur_msg.io_unit = shared_from_this();
	((IoService_Win*)(_io_service.get()))->PostIoMsg(_cur_msg);

	return true;
}


// 操作完成通知
void TcpAcceptor_Win::OnEvent(IoEvent * io_evt)
{
    assert(io_evt == &_evt_accept);

    io_evt->io_unit.reset();
    if (!_running)
    {
        return;
    }

    char * err_msg = nullptr;

    // 发生错误
    if (io_evt->err)
    {
        closesocket(_accept_sock);
        _accept_sock = INVALID_SOCKET;
		CloseAndNotify(io_evt->err);
        return;
    }

    SOCKADDR_IN * local_addr_in = nullptr;
    SOCKADDR_IN * remote_addr_in = nullptr;
    // 解析IP地址
    ConvertIpAddress(&local_addr_in, &remote_addr_in);

    SocketAddr local_addr(local_addr_in->sin_addr.S_un.S_addr, local_addr_in->sin_port);
    SocketAddr remote_addr(remote_addr_in->sin_addr.S_un.S_addr, remote_addr_in->sin_port);

    // 创建
    std::shared_ptr<TcpSocket_Win> sock = TcpSocket_Win::Create(_io_service, _accept_sock, &local_addr, &remote_addr);

    if (!((IoService_Win*)(_io_service.get()))->RegistSocket(*sock))
    {
        sock->Close();
        sock.reset();
		if (_monitor)
		{
			_monitor->OnAccept(nullptr, Error(GetLastError()));
		}
    }
    else
    {
		if (_monitor)
		{
			_monitor->OnAccept(sock, ErrorSuccess);
		}
    }

    // 继续接收
    Error err = Accept();
    if (err)
    {
		CloseAndNotify(err);
    }
}

// IO消息
void TcpAcceptor_Win::OnMsg(IoMsg * io_msg)
{
	assert(io_msg == &_cur_msg && _running.load() == false);
	
	_cur_msg.io_unit.reset();

	if (io_msg->msg_type == kIoMsgType_Close)
	{
		closesocket(_sock);
		_sock = INVALID_SOCKET;

		if (_accept_sock != INVALID_SOCKET)
		{
			closesocket(_accept_sock);
			_accept_sock = INVALID_SOCKET;
		}

		if (_monitor)
		{
			_monitor->OnClosed(ErrorSuccess);
		}
	}
}


// 开始接收连接
Error TcpAcceptor_Win::Accept()
{
    if (!_running)
    {
        return false;
    }

    // 新建一个套接字用于接收新的连接
    _accept_sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (_accept_sock == INVALID_SOCKET)
    {
        return Error(GetLastError());
    }

    // 事件中关联自身
    _evt_accept.io_unit = shared_from_this();

    // 投递一个连接操作
    DWORD dwTrans = 0;
    if (!this->_lpfn_acceptex(_sock, _accept_sock, _addr_buf, 0,
        sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &dwTrans, (OVERLAPPED*)&_evt_accept))
    {
        DWORD err = WSAGetLastError();
        if (err != ERROR_IO_PENDING)
        {
            _evt_accept.io_unit.reset();
            closesocket(_accept_sock);
            _accept_sock = INVALID_SOCKET;
            return err;
        }
    }

    return ErrorSuccess;
}

// 转换ip地址
void TcpAcceptor_Win::ConvertIpAddress(SOCKADDR_IN ** local_addr, SOCKADDR_IN ** remote_addr)
{
    // 加载GetAcceptExSockaddrs函数指针
    if (this->_lpfn_get_acceptex_sockaddrs == nullptr)
    {
        GUID guid_accept_ex = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD recv_bytes;
        if (WSAIoctl(this->_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_accept_ex, sizeof(guid_accept_ex),
            &this->_lpfn_get_acceptex_sockaddrs, sizeof(this->_lpfn_get_acceptex_sockaddrs), &recv_bytes, nullptr, nullptr)
            == INVALID_SOCKET)
        {
            return;
        }
    }

    int local_len, remote_len;
    // 转换地址
    this->_lpfn_get_acceptex_sockaddrs(this->_addr_buf, 0, sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16,
        (SOCKADDR**)local_addr, &local_len, (SOCKADDR**)remote_addr, &remote_len);
}

// 关闭并通知
void TcpAcceptor_Win::CloseAndNotify(Error err)
{
	bool temp = true;
	if (!_running.compare_exchange_weak(temp, false))
	{
		return;
	}

	closesocket(_sock);
	_sock = INVALID_SOCKET;

	if (_accept_sock != INVALID_SOCKET)
	{
		closesocket(_accept_sock);
		_accept_sock = INVALID_SOCKET;
	}

	if (_monitor)
	{
		_monitor->OnClosed(err);
	}
}
