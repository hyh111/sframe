
#ifndef SFRAME_BLOCKING_QUEUE_H
#define SFRAME_BLOCKING_QUEUE_H

#include "RingQueue.h"
#include "ConditionVariable.h"

namespace sframe {

// 阻塞队列
template<typename T>
class BlockingQueue
{
public:

	static const int32_t kDefaultInitCapacity = 32;

	static const int32_t kDefaultIncSize = 8;

	BlockingQueue(int32_t init_capacity = kDefaultInitCapacity, int32_t inc_size = kDefaultIncSize) 
		: _ring_queue(init_capacity, inc_size), _stop(false) {}

	~BlockingQueue() {}

	void Push(const T & val)
	{
		AutoLock l(_lock);
		if (_stop)
		{
			return;
		}

		_ring_queue.Push(val);
		// 唤醒一个在等待的线程
		_cond.WakeUpOne();
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
	RingQueue<T> _ring_queue;
	Lock _lock;
	ConditionVariable _cond;
	bool _stop;
};

}

#endif