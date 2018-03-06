
#ifndef SFRAME_IO_SERVICE_LINUX_H
#define SFRAME_IO_SERVICE_LINUX_H

#include <atomic>
#include <vector>
#include "../IoService.h"
#include "IoUnit.h"
#include "../../util/Lock.h"

namespace sframe {

// Linux下的Io服务(采用EPOLL实现)
class IoService_Linux : public IoService
{
public:
	// epoll等待最大事件数量
	static const int kMaxEpollEventsNumber = 1024;
	// IO消息缓冲区长度
	static const int kMaxIoMsgBufferSize = 65536;

public:
	IoService_Linux();

	virtual ~IoService_Linux();

	Error Init() override;

	void RunOnce(int32_t wait_ms, Error & err) override;

	void Close() override;

	// 添加监听事件
	bool AddIoEvent(const IoUnit & iounit, const IoEvent ioevt);

	// 修改监听事件
	bool ModifyIoEvent(const IoUnit & iounit, const IoEvent ioevt);

	// 删除监听事件
	bool DeleteIoEvent(const IoUnit & iounit, const IoEvent ioevt);

	// 投递消息
	void PostIoMsg(const IoMsg & io_msg);

private:
	int _epoll_fd;
	int _msg_evt_fd;               // 用于实现IO消息的发送与处理
	std::vector<IoMsg*> _msgs;     // IO消息列表
	sframe::Lock _msgs_lock;  // 消息列表锁
};

}

#endif
