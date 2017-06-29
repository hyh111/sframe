
#ifndef SFRAME_RING_QUEUE_H
#define SFRAME_RING_QUEUE_H

#include <inttypes.h>
#include <string.h>
#include <algorithm>
#include <type_traits>

namespace sframe {

// 拷贝数据
template<typename T, bool>
struct CopyDataHelper
{
	static void Copy(T * dst, const T * src, size_t len)
	{
		memcpy(dst, src, sizeof(dst[0]) * len);
	}
};

template<typename T>
struct CopyDataHelper<T, false>
{
	static void Copy(T * dst, const T * src, size_t len)
	{
		for (size_t i = 0; i < len; i++)
		{
			dst[i] = src[i];
		}
	}
};

// 环形队列
template<typename T>
class RingQueue
{
public:

	static const int32_t kMaxCapacity = 0x7fffffff;

	static const int32_t kDefaultInitCapacity = 32;

	static const int32_t kDefaultIncSize = 8;

	RingQueue(int32_t init_capacity = kDefaultInitCapacity, int32_t inc_size = kDefaultIncSize)
		: _head(0), _tail(0), _len(0)
	{
		_capacity = std::max(1, init_capacity);
		_inc_size = std::max(1, inc_size);
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
		
		size_t first_copt_len = _capacity - _head;
		CopyData(new_queue, _queue + _head, first_copt_len);
		CopyData(new_queue + first_copt_len, _queue, _head);
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
				CopyData(_queue + _head, q._queue + _head, _len);
			}
			else
			{
				assert((_capacity - _head) + _tail == _len);
				CopyData(_queue + _head, q._queue + _head, _capacity - _head);
				CopyData(_queue, q._queue, _tail);
			}
		}
	}

	void CopyData(T * dst, const T * src, size_t len)
	{
		if (len > 0)
		{
			sframe::CopyDataHelper<T, std::is_pod<T>::value>::Copy(dst, src, len);
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
