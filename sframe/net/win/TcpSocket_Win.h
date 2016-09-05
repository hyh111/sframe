
#ifndef SFRAME_TCP_SOCKET_WIN_H
#define SFRAME_TCP_SOCKET_WIN_H

#include <WinSock2.h>
#include <MSWSock.h>
#include "../TcpSocket.h"
#include "IoUnit.h"

namespace sframe{

class TcpSocket_Win : public IoUnit, public TcpSocket, public std::enable_shared_from_this <TcpSocket_Win>
{
public:
    // 接收缓冲区大小
    static const int32_t kRecvBufSize = 65536;

public:

	// 创建Socket对象
	static std::shared_ptr<TcpSocket_Win> Create(const std::shared_ptr<IoService> & ioservice, SOCKET sock,
		const SocketAddr * local_addr, const SocketAddr * remote_addr);

public:
    TcpSocket_Win(const std::shared_ptr<IoService> & ioservice);

    virtual ~TcpSocket_Win() {}

    // 连接
    void Connect(const SocketAddr & remote) override;

    // 发送数据
    void Send(const char * data, int32_t len) override;

    // 开始接收数据(外部只能调用一次)
    void StartRecv() override;

    // 关闭
	bool Close() override;

	// 设置TCP_NODELAY
	Error SetTcpNodelay(bool on) override;

    // 完成事件通知
    void OnEvent(IoEvent * io_evt) override;

    // IO消息通知
    void OnMsg(IoMsg * io_msg) override;

private:
    // 发送完成
    void SendCompleted(Error err, int32_t data_len);

    // 接收完成
    void RecvCompleted(Error err, int32_t data_len);

    // 连接完成
    void ConnectCompleted(Error err);

    // 发送数据
    bool SendData(char * buf, int32_t len);

    // 接收
    bool RecvData();

    // 关闭并通知
    void CloseAndNotify(Error err);

private:
    LPFN_CONNECTEX _lpfn_connectex;
    IoEvent _evt_connect;
    IoEvent _evt_send;
    IoEvent _evt_recv;
    IoMsg _cur_msg;
	DWORD _last_error_code;
    int32_t _sending_len;                // 正在发送中的数据长度
    char _recv_buf[kRecvBufSize];        // 接收缓冲区
    int32_t _recv_len;                   // 接收缓冲区中数据长度
	bool _tcp_nodelay;
};

}

#endif