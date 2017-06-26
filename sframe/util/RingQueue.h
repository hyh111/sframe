
#ifndef SFRAME_RING_QUEUE_H
#define SFRAME_RING_QUEUE_H

#include <inttypes.h>
#include <algorithm>

namespace sframe {

// 环形队列
template<typename T>
class RingQueue
{
public:

	static const int32_t kMaxCapacity = 0x7fffffff;

	static const int32_t kDefaultInitCapacity = 32;

	static const int32_t kDefaultIncSize = 8;

	RingQueue(int32_t init_capacity = kDefaultInitCapacity, int32_t inc_size = kDefaultIncSize)
		: _capacity(std::max(1, init_capacity)), _inc_size(std::max(1, inc_size)), _head(0), _tail(0), _len(0)
	{
		_queue = new T[_capacity];
	}

	RingQueue(const RingQueue & q)
	{
		_queue = nullptr;
		CopyFromOther(q);
	}

	~RingQueue()
	{
		delete[] _queue;
	}

	RingQueue & operator= (const RingQueue & q)
	{
		CopyFromOther(q);
		return (*this);
	}

	// 压入
	void Push(const T & val)
	{
		if (_len >= _capacity)
		{
			assert(_len == _capacity);
			_len = _capacity;
			IncreaseCapacity();
		}

		if (_len < _capacity)
		{
			_queue[_tail] = val;
			_tail = (_tail + 1) % _capacity;
			_len++;
		}
	}

	// 弹出
	bool Pop(T * val = nullptr)
	{
		if (_len <= 0)
		{
			return false;
		}

		if (val)
		{
			(*val) = _queue[_head];
		}
		_head = (_head + 1) % _capacity;
		_len--;

		return true;
	}

private:

	void IncreaseCapacity()
	{
		assert(_len == _capacity && _head == _tail);
		if (kMaxCapacity - _inc_size < _capacity)
		{
			return;
		}

		int32_t new_capacity = _capacity + _inc_size;
		T * new_queue = new T[new_capacity];
		int32_t cur_copy_index = 0;

		for (int32_t i = _head; i < _capacity; i++)
		{
			new_queue[cur_copy_index++] = _queue[i];
		}

		for (int32_t i = 0; i < _head; i++)
		{
			new_queue[cur_copy_index++] = _queue[i];
		}

		assert(cur_copy_index == _len);

		delete[] _queue;
		_queue = new_queue;
		_capacity = new_capacity;
		_head = 0;
		_tail = _len;
	}

	void CopyFromOther(const RingQueue & q)
	{
		assert(q._queue && q._capacity > 0 && q._inc_size > 0 && q._head < q._capacity && q._tail < q._capacity);
		if (_queue)
		{
			delete[] _queue;
		}
		_queue = new T[q._capacity];
		_head = q._head;
		_tail = q._tail;
		_len = q._len;
		_capacity = q._capacity;
		_inc_size = q._inc_size;
		if (_len > 0)
		{
			if (_head < _tail)
			{
				assert(_tail - _head == _len);
				for (int i = _head; i < _tail; i++)
				{
					_queue[i] = q._queue[i];
				}
			}
			else
			{
				assert((_capacity - _head) + _tail == _len);
				for (int i = _head; i < _capacity; i++)
				{
					_queue[i] = q._queue[i];
				}
				for (int i = 0; i < _tail; i++)
				{
					_queue[i] = q._queue[i];
				}
			}
		}
	}

private:
	T * _queue;
	int32_t _head;
	int32_t _tail;
	int32_t _len;
	int32_t _capacity;
	int32_t _inc_size;
};

}

#endif
