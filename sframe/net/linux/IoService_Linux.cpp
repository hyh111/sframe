
#include <assert.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "IoService_Linux.h"

using namespace sframe;


std::shared_ptr<IoService> IoService::Create()
{
	std::shared_ptr<IoService> ioservice = std::make_shared<IoService_Linux>();
	return ioservice;
}



IoService_Linux::IoService_Linux()
{
	_epoll_fd = -1;
	_msg_evt_fd = -1;
}

IoService_Linux::~IoService_Linux()
{
	if (_epoll_fd > 0)
	{
		close(_epoll_fd);
	}
	if (_msg_evt_fd > 0)
	{
		close(_msg_evt_fd);
	}
}

Error IoService_Linux::Init()
{
	_epoll_fd = epoll_create(1);
	if (_epoll_fd < 0)
	{
		goto ERROR_HANDLE;
	}

	_msg_evt_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (_msg_evt_fd < 0)
	{
		goto ERROR_HANDLE;
	}

	epoll_event evt;
	evt.events = EPOLLIN | EPOLLET;
	evt.data.ptr = nullptr;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _msg_evt_fd, &evt) != 0)
	{
		goto ERROR_HANDLE;
	}

	_open = true;
	return ErrorSuccess;

ERROR_HANDLE:
	
	Error err = errno;

	if (_msg_evt_fd > 0)
	{
		close(_msg_evt_fd);
		_msg_evt_fd = -1;
	}

	if (_epoll_fd > 0)
	{
		close(_epoll_fd);
		_epoll_fd = -1;
	}

	return err;
}

void IoService_Linux::RunOnce(int32_t wait_ms, Error & err)
{
	err = ErrorSuccess;

	if (!_open)
	{
		return;
	}

    epoll_event evts[kMaxEpollEventsNumber];
    int num = epoll_wait(_epoll_fd, evts, kMaxEpollEventsNumber, wait_ms);
    if (num < 0)
    {
        // 发生错误
		if (errno != EINTR)
		{
			err = errno;
		}

        return;
    }

    // 遍历所有事件
    for (int i = 0; i < num; ++i)
    {
        epoll_event * cur_evt = &evts[i];
        IoUnit * sock_ptr = (IoUnit*)cur_evt->data.ptr;
        if (sock_ptr == nullptr)
        {
			_msgs_lock.lock();

            // 读数据
            uint64_t c = 0;
			read(_msg_evt_fd, &c, sizeof(c));
            // 取出所有消息
            std::vector<IoMsg*> msgs;
            _msgs.swap(msgs);

            _msgs_lock.unlock();

			// 处理所有消息
            for (IoMsg * m : msgs)
            {
				if (m)
				{
					std::shared_ptr<IoUnit> s = m->io_unit;
					if (s)
					{
						s->OnMsg(m);
					}
				}
				else
				{
					_open = false;
					return;
				}
            }
        }
        else
        {
            // 通知
            sock_ptr->OnEvent(cur_evt->events);
        }
    }
}

void IoService_Linux::Close()
{
	if (!_open)
	{
		return;
	}

	AutoLock l(_msgs_lock);
	_msgs.push_back(nullptr);
	uint64_t c = 1;
	int ret = write(_msg_evt_fd, &c, sizeof(c));
	assert(ret >= sizeof(c));
}

// 添加监听事件
bool IoService_Linux::AddIoEvent(const IoUnit & iounit, const IoEvent ioevt)
{
    epoll_event evt;
    evt.data.ptr = (void *)&iounit;
    evt.events = ioevt;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, iounit.GetSocket(), &evt) < 0)
    {
        return false;
    }

    return true;
}

// 修改监听事件
bool IoService_Linux::ModifyIoEvent(const IoUnit & iounit, const IoEvent ioevt)
{
    epoll_event evt;
    evt.data.ptr = (void *)&iounit;
    evt.events = ioevt;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, iounit.GetSocket(), &evt) < 0)
    {
        return false;
    }

    return true;
}

// 删除监听事件
bool IoService_Linux::DeleteIoEvent(const IoUnit & iounit, const IoEvent ioevt)
{
    epoll_event evt;
    evt.data.ptr = (void *)&iounit;
    evt.events = ioevt;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, iounit.GetSocket(), &evt) < 0)
    {
        return false;
    }

    return true;
}

// 投递消息
void IoService_Linux::PostIoMsg(const IoMsg & io_msg)
{
	AutoLock l(_msgs_lock);
    _msgs.push_back((IoMsg*)&io_msg);
	uint64_t c = 1;
	int ret = write(_msg_evt_fd, &c, sizeof(c));
	assert(ret >= sizeof(c));
}
