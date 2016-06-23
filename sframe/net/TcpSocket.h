
#ifndef SFRAME_TCP_SOCKET_H
#define SFRAME_TCP_SOCKET_H

#include <atomic>
#include <memory>
#include "SocketAddr.h"
#include "Error.h"
#include "IoService.h"
#include "SendBuffer.h"

namespace sframe{

// TcpSocket类
class TcpSocket
{
public:

    // 状态
    enum State : int32_t
    {
        kState_Initial,    // 初始状态
        kState_Connecting, // 连接中
        kState_Opened,     // 开启
        kState_Closed,     // 已关闭
    };

    // Tcp套接字监听器接口，实现此接口监听套接字的事件
    class Monitor
    {
    public:
        // 接收到数据
		// 返回剩余多少数据
        virtual int32_t OnReceived(char * data, int32_t len) = 0;

        // Socket关闭
		// by_self: true表示主动请求的关闭操作
        virtual void OnClosed(bool by_self, sframe::Error err) = 0;

        // 连接操作完成
        virtual void OnConnected(sframe::Error err) {}

    };

public:
    // 创建Socket对象
    static std::shared_ptr<TcpSocket> Create(const std::shared_ptr<IoService> & ioservice);

public:
    TcpSocket() : _monitor(nullptr)
    {
        _state.store(kState_Initial);
    }

    virtual ~TcpSocket() {}

    // 连接
    virtual void Connect(const SocketAddr & remote) = 0;

    // 发送数据
    virtual void Send(const char * data, int32_t len) = 0;

    // 开始接收数据(设置监听器后调用一次)
    virtual void StartRecv() = 0;

    // 关闭
	// 返回true: 表示成功开始执行关闭操作，只有处于kState_Connecting和kState_Opened的socket才会执行关闭，关闭完成后会执行OnClosed回调
	// 返回false: 表示socket本来就没有打开(处于kState_Initial或kState_Closed状态)，不会执行关闭操作，也不会执行OnClosed回调
    virtual bool Close() = 0;

    // 设置监听器
    void SetMonitor(Monitor * monitor)
    {
        _monitor = monitor;
    }

    // 获取本地地址
    const SocketAddr & GetLocalAddress() const
    {
        return _local_addr;
    }

    // 获取远程地址
    const SocketAddr & GetRemoteAddress() const
    {
        return _remote_addr;
    }

    // 获取状态
    State GetState() const
    {
        return (State)_state.load();
    }

    // 是否开启
    bool IsOpen() const
    {
        return _state.load() == kState_Opened;
    }

protected:
	SocketAddr _local_addr;
	SocketAddr _remote_addr;
    Monitor * _monitor;                                   // 监听器
    std::atomic_int _state;                               // 状态
	SendBuffer _send_buf;                                 // 发送缓冲区
};

}

#endif