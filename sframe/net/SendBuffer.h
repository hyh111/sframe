
#ifndef SFRAME_TCP_SEND_BUFFER_H
#define SFRAME_TCP_SEND_BUFFER_H

#include <inttypes.h>
#include <assert.h>
#include <memory.h>
#include <list>
#include "../util/Lock.h"

namespace sframe{

// 数据流缓冲区
template<int32_t Capacity>
class StreamBuffer
{
public:
	StreamBuffer() : _len(0), _head(0), _tail(0) {}
	~StreamBuffer() {}

	int32_t Push(const char * data, int32_t data_len)
	{
		if (data == nullptr || data_len <= 0)
		{
			return 0;
		}

		// 检查剩余容量
		int32_t surplus = Capacity - _len;
		assert(surplus >= 0);
		if (surplus <= 0)
		{
			return 0;
		}

		int32_t len = surplus > data_len ? data_len : surplus;
		_len += len;

		if (_tail >= _head)
		{
			int32_t first_len = Capacity - _tail;
			first_len = first_len > len ? len : first_len;
			memcpy(_buf + _tail, data, first_len);

			int32_t second_len = len - first_len;
			assert(second_len <= _head);
			if (second_len > 0)
			{
				memcpy(_buf, data + first_len, second_len);
			}
		}
		else
		{
			assert(_head - _tail >= len);
			memcpy(_buf + _tail, data, len);
		}

		_tail = (_tail + len) % Capacity;

		return len;
	}

	// 读数据
	char * Peek(int32_t & len)
	{
		if (_len == 0)
		{
			len = 0;
			return nullptr;
		}

		if (_head >= _tail)
		{
			len = Capacity - _head;
		}
		else
		{
			len = _tail - _head;
		}

		assert(len <= _len);

		return _buf + _head;
	}

	// 释放空间
	void Free(int32_t len)
	{
		len = len > _len ? _len : len;
		_head = (_head + len) % Capacity;
		_len -= len;
	}

	bool IsFull() const
	{
		return _len >= Capacity;
	}

	bool IsEmpty() const
	{
		return _len <= 0;
	}

private:
	char _buf[Capacity];
	int32_t _len;
	int32_t _head;
	int32_t _tail;
};

// Socket发送缓冲区
class SendBuffer
{
	static const int32_t kBufferCapacity = 65536;
	static const int32_t kStandbyCapacity = 1024 * 8;

public:
	SendBuffer() : _sending(false) {}

	~SendBuffer();

	void Push(const char * data, int32_t len, bool & send_now);

	void PushNotSend(const char * data, int32_t len);

	// 读数据
	char * Peek(int32_t & len);

	// 释放空间
	void Free(int32_t len);

private:
	StreamBuffer<kStandbyCapacity> * GetStandbyBuffer();

private:
	Lock _locker;
	bool _sending;
	StreamBuffer<kBufferCapacity> _buf;
	std::list<StreamBuffer<kStandbyCapacity>*> _standby_list;  // 备用缓冲区链表
};

}

#endif
