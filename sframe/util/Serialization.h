
#ifndef SFRAME_SERIALIZATION_H
#define SFRAME_SERIALIZATION_H

#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <memory>
#include <type_traits>

namespace sframe{

class StreamWriter
{
public:
	StreamWriter(char * buf, uint32_t len) : _buf(buf), _capacity(len), _data_pos(0) {}

	bool Write(const void * data, uint32_t len)
	{
		if (len == 0 || _data_pos + len > _capacity)
		{
			return false;
		}

		memcpy(_buf + _data_pos, data, len);
		_data_pos += len;

		return true;
	}

	uint32_t GetStreamLength() const
	{
		return _data_pos;
	}

	const char * GetStream() const
	{
		return _buf;
	}

private:
	char * const _buf;       // 缓冲区
	uint32_t _capacity;      // 容量
	uint32_t _data_pos;      // 数据当前位置
};

class StreamReader
{
public:
	StreamReader(const char * buf, uint32_t len) : _buf(buf), _capacity(len), _cur_pos(0) {}

	bool Read(void * data, uint32_t len)
	{
		if (_cur_pos + len > _capacity)
		{
			return false;
		}

		memcpy(data, _buf + _cur_pos, len);
		_cur_pos += len;

		return true;
	}

	bool Read(std::string & s, uint32_t len)
	{
		if (_cur_pos + len > _capacity)
		{
			return false;
		}

		s.append(_buf + _cur_pos, len);
		_cur_pos += len;

		return true;
	}

	// 获取已读取的长度
	uint32_t GetReadedLength() const
	{
		return _cur_pos;
	}

private:
	const char * const _buf;  // 缓冲区
	uint32_t _cur_pos;        // 当前位置
	uint32_t _capacity;       // 容量
};


/**
	大小端判断与转换相关辅助
*/

// 检测CPU字节序，大端返回true
inline bool CheckCpuEndian()
{
	union
	{
		uint16_t num;
		uint8_t arr[2];
	}test;

	test.num = 0x0102;
	return (test.arr[0] == 0x01);
}

// 字节序逆向
#define REVERSE_BYTES_ORDER_16(x) ( \
    (((uint16_t)(x) & 0x00ff) << 8) | \
    (((uint16_t)(x) & 0xff00) >> 8) \
    )

#define REVERSE_BYTES_ORDER_32(x) ( \
    (((uint32_t)(x) & 0x000000ff) << 24) | \
    (((uint32_t)(x) & 0x0000ff00) << 8) | \
    (((uint32_t)(x) & 0x00ff0000) >> 8) | \
    (((uint32_t)(x) & 0xff000000) >> 24) \
    )

#define REVERSE_BYTES_ORDER_64(x) ( \
    (((uint64_t)(x) & 0x00000000000000ff) << 56) | \
    (((uint64_t)(x) & 0x000000000000ff00) << 40) | \
    (((uint64_t)(x) & 0x0000000000ff0000) << 24) | \
    (((uint64_t)(x) & 0x00000000ff000000) << 8) | \
    (((uint64_t)(x) & 0x000000ff00000000) >> 8) | \
    (((uint64_t)(x) & 0x0000ff0000000000) >> 24) | \
    (((uint64_t)(x) & 0x00ff000000000000) >> 40) | \
    (((uint64_t)(x) & 0xff00000000000000) >> 56) \
    )

#define HTON_16(x) (sframe::CheckCpuEndian() ? (uint16_t)(x) : REVERSE_BYTES_ORDER_16(x))
#define HTON_32(x) (sframe::CheckCpuEndian() ? (uint32_t)(x) : REVERSE_BYTES_ORDER_32(x))
#define HTON_64(x) (sframe::CheckCpuEndian() ? (uint64_t)(x) : REVERSE_BYTES_ORDER_64(x))
#define NTOH_16(x) (sframe::CheckCpuEndian() ? (uint16_t)(x) : REVERSE_BYTES_ORDER_16(x))
#define NTOH_32(x) (sframe::CheckCpuEndian() ? (uint32_t)(x) : REVERSE_BYTES_ORDER_32(x))
#define NTOH_64(x) (sframe::CheckCpuEndian() ? (uint64_t)(x) : REVERSE_BYTES_ORDER_64(x))

template<typename T>
struct Serializer
{
	static bool Encode(StreamWriter & stream_writer, const T & v)
	{
		assert(false);
		return false;
	}

	static bool Decode(StreamReader & stream_reader, T & v)
	{
		assert(false);
		return false;
	}

	static int32_t GetSize(const T & v)
	{
		assert(false);
		return 0;
	}
};

class Encoder
{
public:
	template<typename T>
	static bool Encode(StreamWriter & stream_writer, const T & obj)
	{
		return call<T>(decltype(match<T>(nullptr))(), stream_writer, obj);
	}

private:
	template<typename T>
	static bool call(std::false_type, StreamWriter & stream_writer, const T & obj)
	{
		return Serializer<T>::Encode(stream_writer, obj);
	}

	template<typename T>
	static bool call(std::true_type, StreamWriter & stream_writer, const T & obj)
	{
		return obj.Encode(stream_writer);
	}

	// 匹配器 ———— bool返回值类成员函数，形如 bool T_Obj::FillObject(T_Reader & reader)
	template<typename U, bool(U::*)(StreamWriter &) const>
	struct MethodMatcher;

	template<typename U>
	static std::true_type match(MethodMatcher<U, &U::Encode>*);

	template<typename U>
	static std::false_type match(...);
};

class Decoder
{
public:
	template<typename T>
	static bool Decode(StreamReader & stream_reader, T & obj)
	{
		return call<T>(decltype(match<T>(nullptr))(), stream_reader, obj);
	}

private:
	template<typename T>
	static bool call(std::false_type, StreamReader & stream_reader, T & obj)
	{
		return Serializer<T>::Decode(stream_reader, obj);
	}

	template<typename T>
	static bool call(std::true_type, StreamReader & stream_reader, T & obj)
	{
		return obj.Decode(stream_reader);
	}

	// 匹配器 ———— bool返回值类成员函数，形如 bool T_Obj::FillObject(T_Reader & reader)
	template<typename U, bool(U::*)(StreamReader &)>
	struct MethodMatcher;

	template<typename U>
	static std::true_type match(MethodMatcher<U, &U::Decode>*);

	template<typename U>
	static std::false_type match(...);
};

class SizeGettor
{
public:
	template<typename T>
	static int32_t GetSize(const T & obj)
	{
		return call<T>(decltype(match<T>(nullptr))(), obj);
	}

private:
	template<typename T>
	static int32_t call(std::false_type, const T & obj)
	{
		return Serializer<T>::GetSize(obj);
	}

	template<typename T>
	static int32_t call(std::true_type, const T & obj)
	{
		return obj.GetSize();
	}

	// 匹配器 ———— bool返回值类成员函数，形如 bool T_Obj::FillObject(T_Reader & reader)
	template<typename U, int32_t(U::*)() const>
	struct MethodMatcher;

	template<typename U>
	static std::true_type match(MethodMatcher<U, &U::GetSize>*);

	template<typename U>
	static std::false_type match(...);
};

template<>
struct Serializer<char>
{
	static bool Encode(StreamWriter & stream_writer, char v)
	{
		return stream_writer.Write((const void *)&v, sizeof(char));
	}

	static bool Decode(StreamReader & stream_reader, char & v)
	{
		return stream_reader.Read((void*)&v, sizeof(char));
	}

	static int32_t GetSize(char v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<int8_t>
{
	static bool Encode(StreamWriter & stream_writer, int8_t v)
	{
		return stream_writer.Write((const void *)&v, sizeof(int8_t));
	}

	static bool Decode(StreamReader & stream_reader, int8_t & v)
	{
		return stream_reader.Read((void*)&v, sizeof(int8_t));
	}

	static int32_t GetSize(int8_t v)
	{
		return sizeof(v);
	}
};


template<>
struct Serializer<uint8_t>
{
	static bool Encode(StreamWriter & stream_writer, uint8_t v)
	{
		return stream_writer.Write((const void *)&v, sizeof(uint8_t));
	}

	static bool Decode(StreamReader & stream_reader, uint8_t & v)
	{
		return stream_reader.Read((void*)&v, sizeof(uint8_t));
	}

	static int32_t GetSize(uint8_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<int16_t>
{
	static bool Encode(StreamWriter & stream_writer, int16_t v)
	{
		v = (int16_t)HTON_16(v);
		return stream_writer.Write((const void *)&v, sizeof(int16_t));
	}

	static bool Decode(StreamReader & stream_reader, int16_t & v)
	{
		if (!stream_reader.Read((void*)&v, sizeof(int16_t)))
		{
			return false;
		}
		v = (int16_t)NTOH_16(v);
		return true;
	}

	static int32_t GetSize(int16_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<uint16_t>
{
	static bool Encode(StreamWriter & stream_writer, uint16_t v)
	{
		v = (uint16_t)HTON_16(v);
		return stream_writer.Write((const void *)&v, sizeof(uint16_t));
	}

	static bool Decode(StreamReader & stream_reader, uint16_t & v)
	{
		if (!stream_reader.Read((void*)&v, sizeof(uint16_t)))
		{
			return false;
		}
		v = (uint16_t)NTOH_16(v);
		return true;
	}

	static int32_t GetSize(uint16_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<int32_t>
{
	static bool Encode(StreamWriter & stream_writer, int32_t v)
	{
		v = (int32_t)HTON_32(v);
		return stream_writer.Write((const void *)&v, sizeof(int32_t));
	}

	static bool Decode(StreamReader & stream_reader, int32_t & v)
	{
		if (!stream_reader.Read((void*)&v, sizeof(int32_t)))
		{
			return false;
		}
		v = (int32_t)NTOH_32(v);
		return true;
	}

	static int32_t GetSize(int32_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<uint32_t>
{
	static bool Encode(StreamWriter & stream_writer, uint32_t v)
	{
		v = (uint32_t)HTON_32(v);
		return stream_writer.Write((const void *)&v, sizeof(uint32_t));
	}

	static bool Decode(StreamReader & stream_reader, uint32_t & v)
	{
		if (!stream_reader.Read((void*)&v, sizeof(uint32_t)))
		{
			return false;
		}
		v = (uint32_t)NTOH_32(v);
		return true;
	}

	static int32_t GetSize(uint32_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<int64_t>
{
	static bool Encode(StreamWriter & stream_writer, int64_t v)
	{
		v = (int64_t)HTON_64(v);
		return stream_writer.Write((const void *)&v, sizeof(int64_t));
	}

	static bool Decode(StreamReader & stream_reader, int64_t & v)
	{
		if (!stream_reader.Read((void*)&v, sizeof(int64_t)))
		{
			return false;
		}
		v = (int64_t)NTOH_64(v);
		return true;
	}

	static int32_t GetSize(int64_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<uint64_t>
{
	static bool Encode(StreamWriter & stream_writer, uint64_t v)
	{
		v = (uint64_t)HTON_64(v);
		return stream_writer.Write((const void *)&v, sizeof(uint64_t));
	}

	static bool Decode(StreamReader & stream_reader, uint64_t & v)
	{
		if (!stream_reader.Read((void*)&v, sizeof(uint64_t)))
		{
			return false;
		}
		v = (uint64_t)NTOH_64(v);
		return true;
	}

	static int32_t GetSize(uint64_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<std::string>
{
	static bool Encode(StreamWriter & stream_writer, const std::string & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.length());

		if (!stream_writer.Write((const void *)&len, sizeof(len)))
		{
			return false;
		}

		return len > 0 ? stream_writer.Write((const void *)v.c_str(), (uint32_t)v.length()) : true;
	}

	static bool Decode(StreamReader & stream_reader, std::string & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		len = (uint16_t)NTOH_16(len);

		return len > 0 ? stream_reader.Read(v, len) : true;
	}

	static int32_t GetSize(const std::string & v)
	{
		return sizeof(uint16_t) + (int32_t)v.length();
	}
};

template<typename T, int Array_Size>
struct Serializer<T[Array_Size]>
{
	static bool Encode(StreamWriter & stream_writer, const T(&v)[Array_Size])
	{
		for (int i = 0; i < Array_Size; i++)
		{
			if (!Encoder::Encode<T>(stream_writer, v[i]))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, T(&v)[Array_Size])
	{
		for (int i = 0; i < Array_Size; i++)
		{
			if (!Decoder::Decode<T>(stream_reader, v[i]))
			{
				return false;
			}
		}

		return true;
	}

	static int32_t GetSize(const T(&v)[Array_Size])
	{
		int32_t len = 0;

		for (int i = 0; i < Array_Size; i++)
		{
			len += SizeGettor::GetSize<T>(v[i]);
		}

		return len;
	}
};

template<typename T>
struct Serializer<std::vector<T>>
{
	static bool Encode(StreamWriter & stream_writer, const std::vector<T> & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.size());

		if (!stream_writer.Write((const void *)&len, sizeof(len)))
		{
			return false;
		}

		for (const T & item : v)
		{
			if (!Encoder::Encode<T>(stream_writer, item))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::vector<T> & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		if (len == 0)
		{
			return true;
		}

		len = (uint16_t)NTOH_16(len);

		v.reserve(len);

		for (int i = 0; i < len; i++)
		{
			T item;

			if (!Decoder::Decode<T>(stream_reader, item))
			{
				return false;
			}

			v.push_back(item);
		}

		return true;
	}

	static int32_t GetSize(const std::vector<T> & v)
	{
		int32_t len = sizeof(uint16_t);

		for (const T & item : v)
		{
			len += SizeGettor::GetSize<T>(item);
		}

		return len;
	}
};

template<typename T>
struct Serializer<std::list<T>>
{
	static bool Encode(StreamWriter & stream_writer, const std::list<T> & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.size());

		if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
		{
			return false;
		}

		for (const T & item : v)
		{
			if (!Encoder::Encode<T>(stream_writer, item))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::list<T> & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		len = (uint16_t)NTOH_16(len);

		for (int i = 0; i < len; i++)
		{
			T item;

			if (!Decoder::Decode<T>(stream_reader, item))
			{
				return false;
			}

			v.push_back(item);
		}

		return true;
	}

	static int32_t GetSize(const std::list<T> & v)
	{
		int32_t len = sizeof(uint16_t);

		for (const T & item : v)
		{
			len += SizeGettor::GetSize<T>(item);
		}

		return len;
	}
};


template<typename T>
struct Serializer<std::set<T>>
{
	static bool Encode(StreamWriter & stream_writer, const std::set<T> & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.size());

		if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
		{
			return false;
		}

		for (const T & item : v)
		{
			if (!Encoder::Encode<T>(stream_writer, item))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::set<T> & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		len = (uint16_t)NTOH_16(len);

		for (int i = 0; i < len; i++)
		{
			T item;

			if (!Decoder::Decode<T>(stream_reader, item))
			{
				return false;
			}

			v.insert(item);
		}

		return true;
	}

	static int32_t GetSize(const std::set<T> & v)
	{
		int32_t len = sizeof(uint16_t);

		for (const T & item : v)
		{
			len += SizeGettor::GetSize<T>(item);
		}

		return len;
	}
};

template<typename T>
struct Serializer<std::unordered_set<T>>
{
	static bool Encode(StreamWriter & stream_writer, const std::unordered_set<T> & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.size());

		if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
		{
			return false;
		}

		for (const T & item : v)
		{
			if (!Encoder::Encode<T>(stream_writer, item))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::unordered_set<T> & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		len = (uint16_t)NTOH_16(len);

		for (int i = 0; i < len; i++)
		{
			T item;

			if (!Decoder::Decode<T>(stream_reader, item))
			{
				return false;
			}

			v.insert(item);
		}

		return true;
	}

	static int32_t GetSize(const std::unordered_set<T> & v)
	{
		int32_t len = sizeof(uint16_t);

		for (const T & item : v)
		{
			len += SizeGettor::GetSize<T>(item);
		}

		return len;
	}
};


template<typename T_Key, typename T_Val>
struct Serializer<std::map<T_Key, T_Val>>
{
	static bool Encode(StreamWriter & stream_writer, const std::map<T_Key, T_Val> & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.size());

		if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
		{
			return false;
		}

		for (const auto it : v)
		{
			if (!Encoder::Encode<T_Key>(stream_writer, it.first) || !Encoder::Encode<T_Val>(stream_writer, it.second))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::map<T_Key, T_Val> & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		len = (uint16_t)NTOH_16(len);

		for (int i = 0; i < len; i++)
		{
			T_Key key;
			T_Val val;

			if (!Decoder::Decode<T_Key>(stream_reader, key) || !Decoder::Decode<T_Val>(stream_reader, val))
			{
				return false;
			}

			v.insert(std::make_pair(key, val));
		}

		return true;
	}

	static int32_t GetSize(const std::map<T_Key, T_Val> & v)
	{
		int32_t len = sizeof(uint16_t);

		for (const auto it : v)
		{
			len += SizeGettor::GetSize<T_Key>(it.first);
			len += SizeGettor::GetSize<T_Val>(it.second);
		}

		return len;
	}
};

template<typename T_Key, typename T_Val>
struct Serializer<std::unordered_map<T_Key, T_Val>>
{
	static bool Encode(StreamWriter & stream_writer, const std::unordered_map<T_Key, T_Val> & v)
	{
		uint16_t len = (uint16_t)HTON_16(v.size());

		if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
		{
			return false;
		}

		for (const auto & it : v)
		{
			if (!Encoder::Encode<T_Key>(stream_writer, it.first) || !Encoder::Encode<T_Val>(stream_writer, it.second))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::unordered_map<T_Key, T_Val> & v)
	{
		v.clear();

		uint16_t len = 0;
		if (!stream_reader.Read((void*)&len, sizeof(uint16_t)))
		{
			return false;
		}

		len = (uint16_t)NTOH_16(len);

		for (int i = 0; i < len; i++)
		{
			T_Key key;
			T_Val val;

			if (!Decoder::Decode<T_Key>(stream_reader, key) || !Decoder::Decode<T_Val>(stream_reader, val))
			{
				return false;
			}

			v.insert(std::make_pair(key, val));
		}

		return true;
	}

	static int32_t GetSize(const std::unordered_map<T_Key, T_Val> & v)
	{
		int32_t len = sizeof(uint16_t);

		for (const auto it : v)
		{
			len += SizeGettor::GetSize<T_Key>(it.first);
			len += SizeGettor::GetSize<T_Val>(it.second);
		}

		return len;
	}
};

// 对象指针序列化器
class ObjectPtrSerializer
{
public:
	template<typename T>
	static bool Encode(StreamWriter & stream_writer, const std::shared_ptr<T> & obj)
	{
		return EncodeImp<T>(decltype(match<T>(nullptr))(), stream_writer, obj);
	}

	template<typename T>
	static bool Decode(StreamReader & stream_reader, std::shared_ptr<T> & obj)
	{
		return DecodeImp<T>(decltype(match<T>(nullptr))(), stream_reader, obj);
	}

	template<typename T>
	static int32_t GetSize(const std::shared_ptr<T> & obj)
	{
		return GetSizeImp<T>(decltype(match<T>(nullptr))(), obj);
	}

private:
	template<typename T>
	static bool EncodeImp(std::false_type, StreamWriter & stream_writer, const std::shared_ptr<T> & obj)
	{
		return Encoder::Encode<T>(stream_writer, *obj);
	}

	template<typename T>
	static bool EncodeImp(std::true_type, StreamWriter & stream_writer, const std::shared_ptr<T> & obj)
	{
		uint16_t obj_type = T::GetObjectType(obj.get());
		obj_type = (uint16_t)HTON_16(obj_type);
		if (!stream_writer.Write((const void *)&obj_type, sizeof(obj_type)))
		{
			return false;
		}

		return Encoder::Encode<T>(stream_writer, *obj);
	}

	template<typename T>
	static bool DecodeImp(std::false_type, StreamReader & stream_reader, std::shared_ptr<T> & obj)
	{
		obj = std::make_shared<T>();
		return Decoder::Decode<T>(stream_reader, *obj);
	}

	template<typename T>
	static bool DecodeImp(std::true_type, StreamReader & stream_reader, std::shared_ptr<T> & obj)
	{
		uint16_t obj_type = 0;
		if (!stream_reader.Read((void*)&obj_type, sizeof(obj_type)))
		{
			return false;
		}
		obj_type = (uint16_t)NTOH_16(obj_type);
		// 必须有创建对象的方法，不然直接使其编译错误
		obj = T::CreateObject(obj_type);
		if (!obj)
		{
			assert(false);
			return false;
		}

		return Decoder::Decode<T>(stream_reader, *obj);
	}

	template<typename T>
	static int32_t GetSizeImp(std::false_type, const std::shared_ptr<T> & obj)
	{
		return SizeGettor::GetSize<T>(*obj);
	}

	template<typename T>
	static int32_t GetSizeImp(std::true_type, const std::shared_ptr<T> & obj)
	{
		return sizeof(uint16_t) + SizeGettor::GetSize<T>(*obj);
	}

	// 匹配器
	template<typename U, uint16_t(*)(const U *)>
	struct MethodMatcher;

	template<typename U>
	static std::true_type match(MethodMatcher<U, &U::GetObjectType>*);

	template<typename U>
	static std::false_type match(...);
};

template<typename T>
struct Serializer<std::shared_ptr<T>>
{
	static bool Encode(StreamWriter & stream_writer, const std::shared_ptr<T> & v)
	{
		uint8_t flag = v ? 1 : 0;
		if (!stream_writer.Write((const void *)&flag, sizeof(uint8_t)))
		{
			return false;
		}

		if (v)
		{
			return ObjectPtrSerializer::Encode(stream_writer, v);
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, std::shared_ptr<T> & v)
	{
		uint8_t flag;
		if (!stream_reader.Read((void*)&flag, sizeof(uint8_t)))
		{
			return false;
		}

		v.reset();

		if (flag > 0)
		{
			return ObjectPtrSerializer::Decode(stream_reader, v);
		}

		return true;
	}

	static int32_t GetSize(const std::shared_ptr<T> & v)
	{
		int32_t s = sizeof(uint8_t);
		if (v)
		{
			s += ObjectPtrSerializer::GetSize(v);
		}
		return s;
	}
};

inline bool AutoEncode(StreamWriter & stream_writer)
{
	return true;
}

template<typename T>
inline bool AutoEncode(StreamWriter & stream_writer, const T & t)
{
	return Encoder::Encode<T>(stream_writer, t);
}

template<typename T, typename... T_Args>
inline bool AutoEncode(StreamWriter & stream_writer, const T & t, const T_Args&... args)
{
	return AutoEncode<T>(stream_writer, t) && AutoEncode<T_Args...>(stream_writer, args...);
}

inline bool AutoDecode(StreamReader & stream_writer)
{
	return true;
}

template<typename T>
inline bool AutoDecode(StreamReader & stream_reader, T & t)
{
	return Decoder::Decode<T>(stream_reader, t);
}

template<typename T, typename... T_Args>
inline bool AutoDecode(StreamReader & stream_reader, T & t, T_Args&... args)
{
	return AutoDecode<T>(stream_reader, t) && AutoDecode<T_Args...>(stream_reader, args...);
}

inline int32_t AutoGetSize()
{
	return 0;
}

template<typename T>
inline int32_t AutoGetSize(const T & t)
{
	return SizeGettor::GetSize<T>(t);
}

template<typename T, typename... T_Args>
inline int32_t AutoGetSize(const T & t, const T_Args&... args)
{
	return AutoGetSize<T>(t) + AutoGetSize<T_Args...>(args...);
}

}

// 序列化申明
#define DECLARE_SERIALIZE \
	int32_t GetSize() const; \
	bool Encode(sframe::StreamWriter & stream_writer) const; \
	bool Decode(sframe::StreamReader & stream_reader);

// 序列化申明(虚函数)
#define DECLARE_VIRTUAL_SERIALIZE \
	virtual int32_t GetSize() const; \
	virtual bool Encode(sframe::StreamWriter & stream_writer) const; \
	virtual bool Decode(sframe::StreamReader & stream_reader);

// 序列化申明(存虚函数)
#define DECLARE_PURE_VIRTUAL_SERIALIZE \
	virtual int32_t GetSize() const = 0; \
	virtual bool Encode(sframe::StreamWriter & stream_writer) const = 0; \
	virtual bool Decode(sframe::StreamReader & stream_reader) = 0;

// 序列化定义(写在类或结构体外部)
#define DEFINE_SERIALIZE_OUTER(S, ...) \
	int32_t S::GetSize() const { return sframe::AutoGetSize(__VA_ARGS__); } \
	bool S::Encode(sframe::StreamWriter & stream_writer) const { return sframe::AutoEncode(stream_writer, ##__VA_ARGS__); } \
	bool S::Decode(sframe::StreamReader & stream_reader) { return sframe::AutoDecode(stream_reader, ##__VA_ARGS__);}

// 序列化定义（写在类或结构体内部）
#define DEFINE_SERIALIZE_INNER(...) \
	int32_t GetSize() const { return sframe::AutoGetSize(__VA_ARGS__); } \
	bool Encode(sframe::StreamWriter & stream_writer) const { return sframe::AutoEncode(stream_writer, ##__VA_ARGS__); } \
	bool Decode(sframe::StreamReader & stream_reader) { return sframe::AutoDecode(stream_reader, ##__VA_ARGS__);}

// 序列化定义（虚函数定义、写在类或结构体内部）
#define DEFINE_VIRTUAL_SERIALIZE_INNER(...) \
	virtual int32_t GetSize() const { return sframe::AutoGetSize(__VA_ARGS__); } \
	virtual bool Encode(sframe::StreamWriter & stream_writer) const { return sframe::AutoEncode(stream_writer, ##__VA_ARGS__); } \
	virtual bool Decode(sframe::StreamReader & stream_reader) { return sframe::AutoDecode(stream_reader, ##__VA_ARGS__);}

#endif