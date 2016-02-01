
#ifndef SFRAME_TCP_ACCEPTOR_H
#define SFRAME_TCP_ACCEPTOR_H

#include "TcpSocket.h"
#include "IoService.h"

namespace sframe{

// TCP连接接收器
class TcpAcceptor
{
public:
    // 监听器
    class Monitor
    {
    public:
        // 连接通知
        virtual void OnAccept(std::shared_ptr<TcpSocket> socket, Error err) = 0;

        // 停止
        virtual void OnClosed(Error err) = 0;
    };

    // 创建
    static std::shared_ptr<TcpAcceptor> Create(const std::shared_ptr<IoService> & ioservice);

public:
    TcpAcceptor() : _monitor(nullptr) {}
    virtual ~TcpAcceptor() {}

    // 开始
    virtual Error Start(const SocketAddr & addr) = 0;

    // 停止
	// 返回true: 表示成功开始执行关闭操作，关闭完成后会执行OnClosed回调
	// 返回false: 表示当前已经处于关闭状态，不会执行关闭操作，也不会执行OnClosed回调
    virtual bool Close() = 0;

    void SetMonitor(Monitor * monitor)
    {
        _monitor = monitor;
    }

protected:
    Monitor * _monitor;

};

}

#endif