
#ifndef SFRAME_TCP_ACCEPTOR_WIN_H
#define SFRAME_TCP_ACCEPTOR_WIN_H

#include <WinSock2.h>
#include <MSWSock.h>
#include "../TcpAcceptor.h"
#include "IoUnit.h"

namespace sframe{

// TCP接收器Windows实现
class TcpAcceptor_Win : public IoUnit, public TcpAcceptor, public std::enable_shared_from_this <TcpAcceptor_Win>
{
public:
    TcpAcceptor_Win(const std::shared_ptr<IoService> & ioservice);
    virtual ~TcpAcceptor_Win() {}

    // 开始
    Error Start(const SocketAddr & listen_addr) override;
    // 停止
    bool Close() override;

    // IO事件通知
    void OnEvent(IoEvent * io_evt) override;

	// IO消息
	void OnMsg(IoMsg * io_msg) override;

private:
    // 开始接收连接
    Error Accept();
    // 转换ip地址
    void ConvertIpAddress(SOCKADDR_IN ** local_addr, SOCKADDR_IN ** remote_addr);
	// 关闭并通知
	void CloseAndNotify(Error err);

private:
    LPFN_ACCEPTEX _lpfn_acceptex;  //AcceptEx函数指针
    LPFN_GETACCEPTEXSOCKADDRS _lpfn_get_acceptex_sockaddrs; //GetAcceptExSockaddrs函数指针
    IoEvent _evt_accept;
    std::atomic_bool _running;     // 运行中
    SOCKET _accept_sock;
    char _addr_buf[(sizeof(SOCKADDR_IN)+16) * 2]; // 保存TCP连接双方地址的缓冲区
	IoMsg _cur_msg;                // 当前消息
};

}

#endif