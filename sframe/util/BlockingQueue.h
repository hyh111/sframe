
#ifndef SFRAME_BLOCKING_QUEUE_H
#define SFRAME_BLOCKING_QUEUE_H

#include "RingQueue.h"
#include "ConditionVariable.h"

namespace sframe {

// 阻塞队列
template<typename T, int Queue_Capacity>
class BlockingQueue
{
public:
	BlockingQueue() : _stop(false) {}

	~BlockingQueue() {}

	bool Push(const T & val)
	{
		AutoLock l(_lock);
		if (_stop || !_ring_queue.Push(val))
		{
			return false;
		}
		// 唤醒一个在等待的线程
		_cond.WakeUpOne();
		return true;
	}

	bool Pop(T * val)
	{
		AutoLock l(_lock);
		bool succ = false;
		while (!_stop)
		{
			succ = _ring_queue.Pop(val);
			if (succ)
			{
				break;
			}

			_cond.Wait(l);
		}
		return succ;
	}

	void Stop()
	{
		AutoLock l(_lock);
		_stop = true;
		_cond.WakeUpAll();
	}

private:
	RingQueue<T, Queue_Capacity> _ring_queue;
	Lock _lock;
	ConditionVariable _cond;
	bool _stop;
};

}

#endif