
#ifndef SFRAME_NET_TCP_ACCEPTOR_LINUX_H
#define SFRAME_NET_TCP_ACCEPTOR_LINUX_H

#include "../TcpAcceptor.h"
#include "IoUnit.h"
#include "../../util/Lock.h"

namespace sframe{

// Tcp连接接收器
class TcpAcceptor_Linux : public IoUnit, public TcpAcceptor, public std::enable_shared_from_this <TcpAcceptor_Linux>
{
public:
    TcpAcceptor_Linux(const std::shared_ptr<IoService> & io_service) : IoUnit(io_service), _cur_msg(kIoMsgType_Close)
    {
        _runing.store(false);
    }
    
    ~TcpAcceptor_Linux(){}

    // 开始
    Error Start(const SocketAddr & addr) override;

    // 停止
	bool Close() override;

    // 事件通知
    void OnEvent(IoEvent io_evt) override;

	// IO消息
	void OnMsg(IoMsg * io_msg) override;

private:

    // 接受连接
    void Accept();

    // 关闭并通知
    void CloseAndNotify(Error err);
    
private:
    std::atomic_bool _runing;        // 是否正在运行
	IoMsg _cur_msg;
};

}

#endif