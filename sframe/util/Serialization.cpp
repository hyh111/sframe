
#include <algorithm>
#include "Serialization.h"

using namespace sframe;

size_t StreamWriter::GetSizeFieldSize(size_t s)
{
	return GetUnsignedNumberSize((uint64_t)s);
}

size_t StreamWriter::GetUnsignedNumberSize(uint64_t n)
{
	size_t ret = 0;

	if (n <= (uint64_t)0xfc)
	{
		ret = sizeof(uint8_t);
	}
	else if (n <= (uint64_t)0xffff)
	{
		ret = sizeof(uint8_t) + sizeof(uint16_t);
	}
	else if (n <= (uint64_t)0xffffffff)
	{
		ret = sizeof(uint8_t) + sizeof(uint32_t);
	}
	else
	{
		ret = sizeof(uint8_t) + sizeof(uint64_t);
	}

	return ret;
}

bool StreamWriter::Write(const void * data, size_t len)
{
	if (len == 0 || _data_pos + len > _capacity)
	{
		return false;
	}

	memcpy(_buf + _data_pos, data, len);
	_data_pos += len;

	return true;
}

bool StreamWriter::WriteSizeField(size_t s)
{
	return WriteUnsignedNumber(s);
}

bool StreamWriter::WriteUnsignedNumber(uint64_t n)
{
	if (n <= (uint64_t)0xfc)
	{
		uint8_t v = (uint8_t)n;
		return Write((const void *)&v, sizeof(v));
	}
	else if (n <= (uint64_t)0xffff)
	{
		uint8_t v1 = 0xfd;
		if (!Write((const void *)&v1, sizeof(v1)))
		{
			return false;
		}

		uint16_t v2 = HTON_16(n);
		return Write((const void *)&v2, sizeof(v2));
	}
	else if (n <= (uint64_t)0xffffffff)
	{
		uint8_t v1 = 0xfe;
		if (!Write((const void *)&v1, sizeof(v1)))
		{
			return false;
		}

		uint32_t v2 = HTON_32(n);
		return Write((const void *)&v2, sizeof(v2));
	}

	uint8_t v1 = 0xff;
	if (!Write((const void *)&v1, sizeof(v1)))
	{
		return false;
	}

	uint64_t v2 = HTON_64(n);
	return Write((const void *)&v2, sizeof(v2));
}


bool StreamReader::Read(void * data, size_t len)
{
	if (_cur_pos + len > _capacity)
	{
		return false;
	}

	memcpy(data, _buf + _cur_pos, len);
	_cur_pos += len;

	return true;
}

bool StreamReader::Read(std::string & s, size_t len)
{
	if (_cur_pos + len > _capacity)
	{
		return false;
	}

	s.append(_buf + _cur_pos, len);
	_cur_pos += len;

	return true;
}

bool StreamReader::ReadSizeField(size_t & s)
{
	uint64_t n = 0;
	if (!ReadUnsignedNumber(n))
	{
		return false;
	}
	s = static_cast<size_t>(n);
	return true;
}

bool StreamReader::ReadUnsignedNumber(uint64_t & n)
{
	uint8_t v1 = 0;
	if (!Read((void*)&v1, sizeof(v1)))
	{
		return false;
	}

	if (v1 <= 0xfc)
	{
		n = v1;
	}
	else if (v1 == 0xfd)
	{
		uint16_t v2 = 0;
		if (!Read((void*)&v2, sizeof(v2)))
		{
			return false;
		}
		n = (uint64_t)NTOH_16(v2);
	}
	else if (v1 == 0xfe)
	{
		uint32_t v2 = 0;
		if (!Read((void*)&v2, sizeof(v2)))
		{
			return false;
		}
		n = (uint64_t)NTOH_32(v2);
	}
	else
	{
		assert(v1 == 0xff);
		uint64_t v2 = 0;
		if (!Read((void*)&v2, sizeof(v2)))
		{
			return false;
		}
		n = (uint64_t)NTOH_64(v2);
	}

	return true;
}

size_t StreamReader::ForwardCurPos(size_t len)
{
	if (_cur_pos > _capacity)
	{
		_cur_pos = _capacity;
	}
	_cur_pos += std::min(len, _capacity - _cur_pos);
	return _cur_pos;
}

size_t StreamReader::BackwardCurPos(size_t len)
{
	if (_cur_pos > _capacity)
	{
		_cur_pos = _capacity;
	}
	_cur_pos -= std::min(_cur_pos, len);
	return _cur_pos;
}

