
#include <assert.h>
#include "TcpSocket_Win.h"
#include "IoService_Win.h"

using namespace sframe;
using namespace sframe;

// 定义TcpSocket类的创建方法
// 创建Socket对象
std::shared_ptr<TcpSocket> TcpSocket::Create(const std::shared_ptr<IoService> & ioservice)
{
	assert(ioservice);
    std::shared_ptr<TcpSocket> obj = std::make_shared<TcpSocket_Win>(ioservice);
    return obj;
}


// 创建Socket对象
std::shared_ptr<TcpSocket_Win> TcpSocket_Win::Create(const std::shared_ptr<IoService> & ioservice, SOCKET sock,
	const SocketAddr * local_addr, const SocketAddr * remote_addr)
{
	assert(ioservice);
	std::shared_ptr<TcpSocket_Win> obj = std::make_shared<TcpSocket_Win>(ioservice);
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

TcpSocket_Win::TcpSocket_Win(const std::shared_ptr<IoService> & ioservice)
    : IoUnit(ioservice), _lpfn_connectex(nullptr), _evt_connect(kIoEvent_ConnectCompleted),
    _evt_send(kIoEvent_SendCompleted), _evt_recv(kIoEvent_RecvCompleted), _cur_msg(kIoMsgType_NotifyError),
    _last_error_code(ERROR_SUCCESS), _recv_len(0), _sending_len(0)
{}

// 连接
void TcpSocket_Win::Connect(const SocketAddr & remote)
{
    int32_t comp = kState_Initial;
    if (!_state.compare_exchange_strong(comp, kState_Connecting))
    {
        return;
    }

    // 新建套接字
    _sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (_sock == INVALID_SOCKET)
    {
        goto END_ERROR;
    }
    
    // 绑定套接字到任意地址
    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(SOCKADDR_IN));
    addr.sin_family = AF_INET;
    if (bind(_sock, (sockaddr *)&addr, sizeof(SOCKADDR_IN)) != 0)
    {
        goto END_ERROR;
    }

    // 加载ConnectEx函数
    if (_lpfn_connectex == nullptr)
    {
        GUID guid_connect_ex = WSAID_CONNECTEX;
        DWORD recv_bytes;
        if (WSAIoctl(this->_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_connect_ex, sizeof(guid_connect_ex),
            &this->_lpfn_connectex, sizeof(this->_lpfn_connectex), &recv_bytes, nullptr, nullptr) == INVALID_SOCKET)
        {
            goto END_ERROR;
        }
    }

    // 注册到IOService
    if (!((IoService_Win*)(_io_service.get()))->RegistSocket(*this))
    {
        goto END_ERROR;
    }

    // 构造远程地址
    SOCKADDR_IN connect_addr;
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = remote.GetPort();
    connect_addr.sin_addr.s_addr = remote.GetIp();

    // 连接事件关联自身
    _evt_connect.io_unit = shared_from_this();

    // 开始连接
    DWORD trans_bytes = 0;
    BOOL ret = this->_lpfn_connectex(this->_sock, (SOCKADDR*)&connect_addr, sizeof(SOCKADDR_IN), nullptr, 0, &trans_bytes, (OVERLAPPED*)&_evt_connect);
    if (!ret)
    {
        DWORD err = WSAGetLastError();
        if (err != ERROR_IO_PENDING)
        {
            goto END_ERROR;
        }
    }

    return;

END_ERROR:

    _last_error_code = GetLastError();

    if (_sock != INVALID_SOCKET)
    {
        closesocket(_sock);
        _sock = INVALID_SOCKET;
    }

    _lpfn_connectex = nullptr;
    _evt_connect.io_unit.reset();

	_cur_msg.msg_type = kIoMsgType_NotifyError;
	_cur_msg.io_unit = shared_from_this();
	((IoService_Win*)(_io_service.get()))->PostIoMsg(_cur_msg);
}

// 发送数据
void TcpSocket_Win::Send(const char * data, int32_t len)
{
    if (len <= 0 || GetState() != kState_Opened)
    {
        return;
    }

	bool send_now = false;
	_send_buf.Push(data, len, send_now);

    if (send_now)
    {
		// 发送数据
		char * buf = _send_buf.Peek(_sending_len);
		assert(_sending_len > 0);

		if (!SendData(buf, _sending_len))
		{
			_last_error_code = GetLastError();

			int32_t comp = TcpSocket::kState_Opened;
			if (!_state.compare_exchange_strong(comp, (int32_t)TcpSocket::kState_Closed))
			{
				return;
			}

			_cur_msg.msg_type = kIoMsgType_NotifyError;
			_cur_msg.io_unit = shared_from_this();
			((IoService_Win*)(_io_service.get()))->PostIoMsg(_cur_msg);
		}
    }
}

// 开始接收数据(外部只需调用一次)
void TcpSocket_Win::StartRecv()
{
    if (GetState() != kState_Opened || _recv_len >= kRecvBufSize)
    {
        return;
    }

    if (!RecvData())
    {
		_last_error_code = GetLastError();

		int32_t comp = TcpSocket::kState_Opened;
		if (!_state.compare_exchange_strong(comp, (int32_t)TcpSocket::kState_Closed))
		{
			return;
		}

		_cur_msg.msg_type = kIoMsgType_NotifyError;
		_cur_msg.io_unit = shared_from_this();
		((IoService_Win*)(_io_service.get()))->PostIoMsg(_cur_msg);
    }
}

// 关闭
bool TcpSocket_Win::Close()
{
    int32_t comp = TcpSocket::kState_Opened;
    if (!_state.compare_exchange_strong(comp, (int32_t)TcpSocket::kState_Closed))
    {
        return false;
    }

	_cur_msg.msg_type = kIoMsgType_Close;
	_cur_msg.io_unit = shared_from_this();
	((IoService_Win*)(_io_service.get()))->PostIoMsg(_cur_msg);

	return true;
}

// 完成事件通知
void TcpSocket_Win::OnEvent(IoEvent * io_evt)
{
    TcpSocket::State s = GetState();
    io_evt->io_unit.reset();
    if (s != TcpSocket::kState_Opened && s != TcpSocket::kState_Connecting)
    {    
        return;
    }

    switch (io_evt->evt_type)
    {
    case kIoEvent_SendCompleted:
        assert(io_evt == &_evt_send);
        SendCompleted(io_evt->err, io_evt->data_len);
        break;

    case kIoEvent_RecvCompleted:
        assert(io_evt == &_evt_recv);
        RecvCompleted(io_evt->err, io_evt->data_len);
        break;

    case kIoEvent_ConnectCompleted:
        assert(io_evt == &_evt_connect);
        ConnectCompleted(io_evt->err);
        break;
    }
}

// IO消息通知
void TcpSocket_Win::OnMsg(IoMsg * io_msg)
{
    assert(io_msg == &_cur_msg);

    TcpSocket::State s = GetState();
	_cur_msg.io_unit.reset();

	if (io_msg->msg_type == kIoMsgType_NotifyError)
	{
		if (s == TcpSocket::kState_Connecting)
		{
			_state.store(TcpSocket::kState_Closed);
			if (_monitor)
			{
				_monitor->OnConnected(_last_error_code);
			}
		}
		else
		{
			shutdown(_sock, SD_BOTH);
			closesocket(_sock);
			_sock = INVALID_SOCKET;
			if (_monitor)
			{
				_monitor->OnClosed(false, _last_error_code);
			}
		}
	}
	else if (io_msg->msg_type == kIoMsgType_Close)
	{
		assert(s == TcpSocket::kState_Closed);
		shutdown(_sock, SD_BOTH);
		closesocket(_sock);
		_sock = INVALID_SOCKET;
		if (_monitor)
		{
			_monitor->OnClosed(true, ErrorSuccess);
		}
	}
}



// 发送完成
void TcpSocket_Win::SendCompleted(Error err, int32_t data_len)
{
    if (err)
    {
		CloseAndNotify(err);
        return;
    }

    // 释放缓冲区
    _send_buf.Free(_sending_len);
    // 取数据
    char * buf = _send_buf.Peek(_sending_len);
    if (_sending_len > 0)
    {
        if (!SendData(buf, _sending_len))
        {
            CloseAndNotify(Error(WSAGetLastError()));
        }
        return;
    }
}

// 接收完成
void TcpSocket_Win::RecvCompleted(Error err, int32_t data_len)
{
    if (err || data_len == 0)
    {
        CloseAndNotify(err);
        return;
    }

    if (_monitor)
    {
        _recv_len += data_len;
        _recv_len = _recv_len > kRecvBufSize ? kRecvBufSize : _recv_len;

        // 通知监听器
        int32_t surplus = _monitor->OnReceived(_recv_buf, _recv_len);
        surplus = surplus > _recv_len ? _recv_len : surplus;
        if (surplus < 0)
        {
            CloseAndNotify(ErrorSuccess);
            return;
        }
        else if (surplus >= kRecvBufSize)
        {
            _recv_len = 0;
        }
        else
        {
            memcpy(this->_recv_buf, this->_recv_buf + (_recv_len - surplus), surplus);
            _recv_len = surplus;
        }
    }

    if (!RecvData())
    {
        CloseAndNotify(Error(WSAGetLastError()));
    }
}

// 连接完成
void TcpSocket_Win::ConnectCompleted(Error err)
{
    if (!err)
    {
        _state.store(TcpSocket::kState_Opened);
    }
    else
    {
		_state.store(TcpSocket::kState_Closed);
        closesocket(_sock);
        _sock = INVALID_SOCKET;
    }

    if (_monitor)
    {
        _monitor->OnConnected(err);
    }
}

// 发送数据
bool TcpSocket_Win::SendData(char * buf, int32_t len)
{
    // 发送事件关联自身
    _evt_send.io_unit = shared_from_this();

    WSABUF wsa_send_buf;
    wsa_send_buf.buf = buf;
    wsa_send_buf.len = len;

    DWORD trans_len = 0;
    if (WSASend(_sock, &wsa_send_buf, 1, &trans_len, 0, (OVERLAPPED*)&_evt_send, nullptr) != 0)
    {
        DWORD err = WSAGetLastError();
        if (err != ERROR_IO_PENDING)
        {
            _evt_send.io_unit.reset();
            return false;
        }
    }

    return true;
}

// 接收
bool TcpSocket_Win::RecvData()
{
    // 接收事件关联自身
    _evt_recv.io_unit = shared_from_this();

    WSABUF wsa_buf;
    wsa_buf.buf = _recv_buf + _recv_len;
    wsa_buf.len = kRecvBufSize - _recv_len;

    DWORD trans = 0, f = 0;
    if (WSARecv(_sock, &wsa_buf, 1, &trans, &f, (OVERLAPPED*)&_evt_recv, nullptr) != 0)
    {
        DWORD err = WSAGetLastError();
        if (err != ERROR_IO_PENDING)
        {
            _evt_recv.io_unit.reset();
            return false;
        }
    }

    return true;
}

// 关闭并通知
void TcpSocket_Win::CloseAndNotify(Error err)
{
    int32_t comp = TcpSocket::kState_Opened;
    if (!_state.compare_exchange_strong(comp, (int32_t)TcpSocket::kState_Closed))
    {
        return;
    }

    shutdown(_sock, SD_BOTH);
    closesocket(_sock);
    _sock = INVALID_SOCKET;

	if (_monitor)
	{
		_monitor->OnClosed(false, err);
	}
}