
#ifndef SFRAME_SERIALIZATION_H
#define SFRAME_SERIALIZATION_H

#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
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

	static size_t GetSizeFieldSize(size_t s);

	StreamWriter(char * buf, size_t len) : _buf(buf), _capacity(len), _data_pos(0) {}

	size_t GetStreamLength() const
	{
		return _data_pos;
	}

	const char * GetStream() const
	{
		return _buf;
	}

	bool Write(const void * data, size_t len);

	bool WriteSizeField(size_t s);

private:
	char * const _buf;       // 缓冲区
	size_t _capacity;      // 容量
	size_t _data_pos;      // 数据当前位置
};

class StreamReader
{
public:
	StreamReader(const char * buf, size_t len) : _buf(buf), _cur_pos(0), _capacity(len) {}

	// 获取已读取的长度
	size_t GetReadedLength() const
	{
		return _cur_pos;
	}

	// 获取未读长度
	size_t GetNotReadLength() const
	{
		return _cur_pos >= _capacity ? 0 : _capacity - _cur_pos;
	}

	const char * GetStreamBuffer() const
	{
		return _buf;
	}

	bool Read(void * data, size_t len);

	bool Read(std::string & s, size_t len);

	bool ReadSizeField(size_t & s);

	size_t ForwardCurPos(size_t len);

	size_t BackwardCurPos(size_t len);

private:
	const char * const _buf;  // 缓冲区
	size_t _cur_pos;        // 当前位置
	size_t _capacity;       // 容量
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

	static size_t GetSize(const T & v)
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
		if (stream_reader.GetNotReadLength() == 0)
		{
			obj = T();
			return true;
		}
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
	static size_t GetSize(const T & obj)
	{
		return call<T>(decltype(match<T>(nullptr))(), obj);
	}

private:
	template<typename T>
	static size_t call(std::false_type, const T & obj)
	{
		return Serializer<T>::GetSize(obj);
	}

	template<typename T>
	static size_t call(std::true_type, const T & obj)
	{
		return obj.GetSize();
	}

	// 匹配器 ———— bool返回值类成员函数，形如 bool T_Obj::FillObject(T_Reader & reader)
	template<typename U, size_t(U::*)() const>
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

	static size_t GetSize(char v)
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

	static size_t GetSize(int8_t v)
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

	static size_t GetSize(uint8_t v)
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

	static size_t GetSize(int16_t v)
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

	static size_t GetSize(uint16_t v)
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

	static size_t GetSize(int32_t v)
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

	static size_t GetSize(uint32_t v)
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

	static size_t GetSize(int64_t v)
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

	static size_t GetSize(uint64_t v)
	{
		return sizeof(v);
	}
};

template<>
struct Serializer<std::string>
{
	static bool Encode(StreamWriter & stream_writer, const std::string & v)
	{
		if (!stream_writer.WriteSizeField(v.length()))
		{
			return false;
		}
		return v.empty() ? true : stream_writer.Write((const void *)v.c_str(), v.length());
	}

	static bool Decode(StreamReader & stream_reader, std::string & v)
	{
		v.clear();

		size_t len = 0;
		if (!stream_reader.ReadSizeField(len))
		{
			return false;
		}

		return len > 0 ? stream_reader.Read(v, len) : true;
	}

	static size_t GetSize(const std::string & v)
	{
		return StreamWriter::GetSizeFieldSize(v.length()) + v.length();
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

	static size_t GetSize(const T(&v)[Array_Size])
	{
		size_t len = 0;

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
		if (!stream_writer.WriteSizeField(v.size()))
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

		size_t len = 0;
		if (!stream_reader.ReadSizeField(len))
		{
			return false;
		}

		if (len == 0)
		{
			return true;
		}

		v.reserve(len);

		for (size_t i = 0; i < len; i++)
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

	static size_t GetSize(const std::vector<T> & v)
	{
		size_t len = StreamWriter::GetSizeFieldSize(v.size());

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
		if (!stream_writer.WriteSizeField(v.size()))
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

		size_t len = 0;
		if (!stream_reader.ReadSizeField(len))
		{
			return false;
		}

		for (size_t i = 0; i < len; i++)
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

	static size_t GetSize(const std::list<T> & v)
	{
		size_t len = StreamWriter::GetSizeFieldSize(v.size());

		for (const T & item : v)
		{
			len += SizeGettor::GetSize<T>(item);
		}

		return len;
	}
};

template<typename T_Set>
struct Serializer_Set
{
	static bool Encode(StreamWriter & stream_writer, const T_Set & v)
	{
		if (!stream_writer.WriteSizeField(v.size()))
		{
			return false;
		}

		for (const typename T_Set::value_type & item : v)
		{
			if (!Encoder::Encode(stream_writer, item))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, T_Set & v)
	{
		v.clear();

		size_t len = 0;
		if (!stream_reader.ReadSizeField(len))
		{
			return false;
		}

		for (size_t i = 0; i < len; i++)
		{
			typename T_Set::value_type item;

			if (!Decoder::Decode(stream_reader, item))
			{
				return false;
			}

			v.insert(item);
		}

		return true;
	}

	static size_t GetSize(const T_Set & v)
	{
		size_t len = StreamWriter::GetSizeFieldSize(v.size());

		for (const typename T_Set::value_type & item : v)
		{
			len += SizeGettor::GetSize(item);
		}

		return len;
	}
};

template<typename T>
struct Serializer<std::set<T>> : Serializer_Set<std::set<T>>
{
};

template<typename T>
struct Serializer<std::unordered_set<T>> : Serializer_Set<std::unordered_set<T>>
{
};

template<typename T>
struct Serializer<std::multiset<T>> : Serializer_Set<std::multiset<T>>
{
};

template<typename T>
struct Serializer<std::unordered_multiset<T>> : Serializer_Set<std::unordered_multiset<T>>
{
};

template<typename T_Map>
struct Serializer_Map
{
	static bool Encode(StreamWriter & stream_writer, const T_Map & v)
	{
		if (!stream_writer.WriteSizeField(v.size()))
		{
			return false;
		}

		for (const auto it : v)
		{
			if (!Encoder::Encode(stream_writer, it.first) || !Encoder::Encode(stream_writer, it.second))
			{
				return false;
			}
		}

		return true;
	}

	static bool Decode(StreamReader & stream_reader, T_Map & v)
	{
		v.clear();

		size_t len = 0;
		if (!stream_reader.ReadSizeField(len))
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			typename T_Map::key_type key;
			typename T_Map::mapped_type val;

			if (!Decoder::Decode(stream_reader, key) || !Decoder::Decode(stream_reader, val))
			{
				return false;
			}

			v.insert(std::make_pair(key, val));
		}

		return true;
	}

	static size_t GetSize(const T_Map & v)
	{
		size_t len = StreamWriter::GetSizeFieldSize(v.size());

		for (const auto it : v)
		{
			len += SizeGettor::GetSize(it.first);
			len += SizeGettor::GetSize(it.second);
		}

		return len;
	}
};

template<typename T_Key, typename T_Val>
struct Serializer<std::map<T_Key, T_Val>> : Serializer_Map<std::map<T_Key, T_Val>>
{
};

template<typename T_Key, typename T_Val>
struct Serializer<std::unordered_map<T_Key, T_Val>> : Serializer_Map<std::unordered_map<T_Key, T_Val>>
{
};

template<typename T_Key, typename T_Val>
struct Serializer<std::multimap<T_Key, T_Val>> : Serializer_Map<std::multimap<T_Key, T_Val>>
{
};

template<typename T_Key, typename T_Val>
struct Serializer<std::unordered_multimap<T_Key, T_Val>> : Serializer_Map<std::unordered_multimap<T_Key, T_Val>>
{
};

template<>
struct Serializer<double>
{
	static bool Encode(StreamWriter & stream_writer, double v)
	{
		std::string s = std::to_string(v);
		return Encoder::Encode<std::string>(stream_writer, s);
	}

	static bool Decode(StreamReader & stream_reader, double & v)
	{
		std::string s;
		if (!Decoder::Decode<std::string>(stream_reader, s))
		{
			return false;
		}
	
		v = strtod(s.c_str(), nullptr);
		return true;
	}

	static size_t GetSize(double v)
	{
		std::string s = std::to_string(v);
		return SizeGettor::GetSize<std::string>(s);
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
	static size_t GetSize(const std::shared_ptr<T> & obj)
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
		uint16_t obj_type = (uint16_t)T::GetObjectType(obj.get());
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
	static size_t GetSizeImp(std::false_type, const std::shared_ptr<T> & obj)
	{
		return SizeGettor::GetSize<T>(*obj);
	}

	template<typename T>
	static size_t GetSizeImp(std::true_type, const std::shared_ptr<T> & obj)
	{
		return sizeof(uint16_t) + SizeGettor::GetSize<T>(*obj);
	}

	// 匹配器
	template<typename U, size_t(*)(const U *)>
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

	static size_t GetSize(const std::shared_ptr<T> & v)
	{
		size_t s = sizeof(uint8_t);
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

inline size_t AutoGetSize()
{
	return 0;
}

template<typename T>
inline size_t AutoGetSize(const T & t)
{
	return SizeGettor::GetSize<T>(t);
}

template<typename T, typename... T_Args>
inline size_t AutoGetSize(const T & t, const T_Args&... args)
{
	return AutoGetSize<T>(t) + AutoGetSize<T_Args...>(args...);
}

}

// 序列化申明
#define DECLARE_SERIALIZE \
	size_t GetSize() const; \
	bool Encode(sframe::StreamWriter & stream_writer) const; \
	bool Decode(sframe::StreamReader & stream_reader);

// 序列化申明(虚函数)
#define DECLARE_VIRTUAL_SERIALIZE \
	virtual size_t GetSize() const; \
	virtual bool Encode(sframe::StreamWriter & stream_writer) const; \
	virtual bool Decode(sframe::StreamReader & stream_reader);

// 序列化申明(存虚函数)
#define DECLARE_PURE_VIRTUAL_SERIALIZE \
	virtual size_t GetSize() const = 0; \
	virtual bool Encode(sframe::StreamWriter & stream_writer) const = 0; \
	virtual bool Decode(sframe::StreamReader & stream_reader) = 0;

// 序列化定义(写在类或结构体外部)
#define DEFINE_SERIALIZE_OUTER(S, ...) \
	size_t S::GetSize() const \
	{ \
		size_t s = sframe::AutoGetSize(__VA_ARGS__); \
		return sframe::StreamWriter::GetSizeFieldSize(s) + s; \
	} \
	bool S::Encode(sframe::StreamWriter & stream_writer) const \
	{ \
		size_t s = sframe::AutoGetSize(__VA_ARGS__);\
		if (!stream_writer.WriteSizeField(s)) {return false;} \
		return sframe::AutoEncode(stream_writer, ##__VA_ARGS__); \
	} \
	bool S::Decode(sframe::StreamReader & stream_reader) \
	{ \
		size_t s = 0; \
		if (!stream_reader.ReadSizeField(s)) {return false;} \
		if (s > stream_reader.GetNotReadLength()) {return false;} \
		sframe::StreamReader sub_stream_reader(stream_reader.GetStreamBuffer() + stream_reader.GetReadedLength(), s); \
		stream_reader.ForwardCurPos(s); \
		return sframe::AutoDecode(sub_stream_reader, ##__VA_ARGS__); \
	}

// 序列化定义（写在类或结构体内部）
#define DEFINE_SERIALIZE_INNER(...) \
	size_t GetSize() const \
	{ \
		size_t s = sframe::AutoGetSize(__VA_ARGS__); \
		return sframe::StreamWriter::GetSizeFieldSize(s) + s; \
	} \
	bool Encode(sframe::StreamWriter & stream_writer) const \
	{ \
		size_t s = sframe::AutoGetSize(__VA_ARGS__);\
		if (!stream_writer.WriteSizeField(s)) {return false;} \
		return sframe::AutoEncode(stream_writer, ##__VA_ARGS__); \
	} \
	bool Decode(sframe::StreamReader & stream_reader) \
	{ \
		size_t s = 0; \
		if (!stream_reader.ReadSizeField(s)) {return false;} \
		if (s > stream_reader.GetNotReadLength()) {return false;} \
		sframe::StreamReader sub_stream_reader(stream_reader.GetStreamBuffer() + stream_reader.GetReadedLength(), s); \
		stream_reader.ForwardCurPos(s); \
		return sframe::AutoDecode(sub_stream_reader, ##__VA_ARGS__); \
	}

// 序列化定义（虚函数定义、写在类或结构体内部）
#define DEFINE_VIRTUAL_SERIALIZE_INNER(...) \
	virtual size_t GetSize() const \
	{ \
		size_t s = sframe::AutoGetSize(__VA_ARGS__); \
		return sframe::StreamWriter::GetSizeFieldSize(s) + s; \
	} \
	virtual bool Encode(sframe::StreamWriter & stream_writer) const \
	{ \
		size_t s = sframe::AutoGetSize(__VA_ARGS__);\
		if (!stream_writer.WriteSizeField(s)) {return false;} \
		return sframe::AutoEncode(stream_writer, ##__VA_ARGS__); \
	} \
	virtual bool Decode(sframe::StreamReader & stream_reader) \
	{ \
		size_t s = 0; \
		if (!stream_reader.ReadSizeField(s)) {return false;} \
		if (s > stream_reader.GetNotReadLength()) {return false;} \
		sframe::StreamReader sub_stream_reader(stream_reader.GetStreamBuffer() + stream_reader.GetReadedLength(), s); \
		stream_reader.ForwardCurPos(s); \
		return sframe::AutoDecode(sub_stream_reader, ##__VA_ARGS__); \
	}

#endif