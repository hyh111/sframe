
#ifndef SFRAME_RING_QUEUE_H
#define SFRAME_RING_QUEUE_H

#include <inttypes.h>
#include "Lock.h"
#include "Singleton.h"

namespace sframe {

// 空锁
class NoneLock
{
public:
	void lock(){}

	void unlock(){}
};

// 环形队列
template<typename T, int Queue_Capacity, typename T_Lock = NoneLock>
class RingQueue : public noncopyable
{
public:
	RingQueue() : _head(0), _tail(0), _len(0) {}

	~RingQueue() {}

	// 压入
	bool Push(const T & val)
	{
		_lock.lock();
		if (_len >= Queue_Capacity)
		{
			_lock.unlock();
			return false;
		}

		_queue[_tail] = val;
		_tail = (_tail + 1) % Queue_Capacity;
		_len++;

		_lock.unlock();
		return true;
	}

	// 弹出
	bool Pop(T * val)
	{
		_lock.lock();
		if (_len <= 0)
		{
			_lock.unlock();
			return false;
		}

		if (val)
		{
			(*val) = _queue[_head];
		}
		_head = (_head + 1) % Queue_Capacity;
		_len--;

		_lock.unlock();
		return true;
	}

private:
	T_Lock _lock;
	T _queue[Queue_Capacity];
	int32_t _head;
	int32_t _tail;
	int32_t _len;
};

}

#endif
