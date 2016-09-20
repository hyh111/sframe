
#ifndef SFRAME_NET_TCP_SOCKET_LINUX_H
#define SFRAME_NET_TCP_SOCKET_LINUX_H

#include "../TcpSocket.h"
#include "../SendBuffer.h"
#include "IoUnit.h"

namespace sframe{

// TCP套接字
class TcpSocket_Linux : public IoUnit, public TcpSocket, public std::enable_shared_from_this <TcpSocket_Linux>
{
public:
    // 接收缓冲区大小
    static const int32_t kRecvBufSize = 65536;

public:
	// 创建Socket对象
	static std::shared_ptr<TcpSocket_Linux> Create(const std::shared_ptr<IoService> & ioservice, int sock,
		const SocketAddr * local_addr, const SocketAddr * remote_addr);

public:

	TcpSocket_Linux(const std::shared_ptr<IoService> & io_service);

    ~TcpSocket_Linux() {}

    // 连接
    void Connect(const SocketAddr & remote) override;

    // 发送数据
    void Send(const char * data, int32_t len) override;

    // 开始接收数据(设置监听器后调用一次)
    void StartRecv() override;

    // 关闭
    bool Close() override;

	// 设置TCP_NODELAY
	Error SetTcpNodelay(bool on) override;

    void OnEvent(IoEvent io_evt) override;

    void OnMsg(IoMsg * io_msg) override;

private:
    // 修改Epoll的等待事件
    bool ModifyEpollEvent(uint32_t evt);

	// 连接
	void Connect();

    // 发送数据
    bool SendData();

    // 接收数据
    bool RecvData();

    // 关闭并通知
    void CloseAndNotify(Error err);

private:
    bool _add_evt;                           // 是否已经添加了事件
    IoMsg _io_msg_send_and_conn;             // IO消息(用于发送和连接操作，发送和连接操作，同时只能存在一个，可以共用)
	IoMsg _io_msg_close;                     // IO消息(用于关闭)
	IoMsg _io_msg_notify_err;                // IO消息(用于通知错误)
	int32_t _last_error;
    uint32_t _cur_events;                    // 当前等待的事件
    char _recv_buf[kRecvBufSize];            // 接收缓冲区
    int32_t _recv_len;                       // 接收的数据长度
	bool _tcp_nodelay;
};

}

#endif