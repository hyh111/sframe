
#include "../util/ObjectPool.h"
#include "SendBuffer.h"

using namespace sframe;

SendBuffer::~SendBuffer()
{
	for (auto standby : _standby_list)
	{
		ObjectPool<StreamBuffer<kStandbyCapacity>>::Instance().Delete(standby);
	}
}

void SendBuffer::Push(const char * data, int32_t len, bool & send_now)
{
	send_now = false;

	AUTO_LOCK(_locker);

	// ÏÈ³¢ÊÔÑ¹ÈëÖ÷»º³åÇø
	int32_t pushed = _buf.Push(data, len);
	if (pushed < len)
	{
		if (pushed > 0)
		{
			// ÈôÑ¹ÈëÁËÊý¾Ýµ½Ö÷»º³åÇø£¬ÄÇÃ´±¸ÓÃµÄÁ´±í±Ø¶¨Îª¿Õ
			assert(_standby_list.empty());
		}

		const char * remain_data = data + pushed;
		int32_t remain_len = len - pushed;

		while (remain_len > 0)
		{
			auto standby = GetStandbyBuffer();
			pushed = standby->Push(remain_data, remain_len);
			remain_data += pushed;
			remain_len -= pushed;
		}
	}

	if (!_sending && !_buf.IsEmpty())
	{
		_sending = true;
		send_now = true;
	}
}

void SendBuffer::PushNotSend(const char * data, int32_t len)
{
	AUTO_LOCK(_locker);

	// ÏÈ³¢ÊÔÑ¹ÈëÖ÷»º³åÇø
	int32_t pushed = _buf.Push(data, len);
	if (pushed < len)
	{
		if (pushed > 0)
		{
			// ÈôÑ¹ÈëÁËÊý¾Ýµ½Ö÷»º³åÇø£¬ÄÇÃ´±¸ÓÃµÄÁ´±í±Ø¶¨Îª¿Õ
			assert(_standby_list.empty());
		}

		const char * remain_data = data + pushed;
		int32_t remain_len = len - pushed;

		while (remain_len > 0)
		{
			auto standby = GetStandbyBuffer();
			pushed = standby->Push(remain_data, remain_len);
			remain_data += pushed;
			remain_len -= pushed;
		}
	}
}

// ¶ÁÊý¾Ý
char * SendBuffer::Peek(int32_t & len)
{
	AUTO_LOCK(_locker);

	char * data = _buf.Peek(len);
	if (data == nullptr)
	{
		assert(len == 0);
		_sending = false;
	}

	return data;
}

// ÊÍ·Å¿Õ¼ä
void SendBuffer::Free(int32_t len)
{
	if (len <= 0)
	{
		return;
	}

	AUTO_LOCK(_locker);
	_buf.Free(len);

	assert(!_buf.IsFull());

	while (!_buf.IsFull() && !_standby_list.empty())
	{
		StreamBuffer<kStandbyCapacity>* standby = _standby_list.front();
		int32_t move_len = 0;
		const char * data = standby->Peek(move_len);
		assert(data && move_len > 0);

		int32_t push_len = _buf.Push(data, move_len);
		assert(push_len > 0);
		standby->Free(push_len);

		if (standby->IsEmpty())
		{
			ObjectPool<StreamBuffer<kStandbyCapacity>>::Instance().Delete(standby);
			_standby_list.pop_front();
		}
	}
}

StreamBuffer<SendBuffer::kStandbyCapacity> * SendBuffer::GetStandbyBuffer()
{
	if (_standby_list.empty() || _standby_list.back()->IsFull())
	{
		StreamBuffer<kStandbyCapacity>* standby = ObjectPool<StreamBuffer<kStandbyCapacity>>::Instance().New();
		assert(standby);
		_standby_list.push_back(standby);
	}

	return _standby_list.back();
}