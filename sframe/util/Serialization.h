
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

#define HTON_16(x) (CheckCpuEndian() ? (uint16_t)(x) : REVERSE_BYTES_ORDER_16(x))
#define HTON_32(x) (CheckCpuEndian() ? (uint32_t)(x) : REVERSE_BYTES_ORDER_32(x))
#define HTON_64(x) (CheckCpuEndian() ? (uint64_t)(x) : REVERSE_BYTES_ORDER_64(x))
#define NTOH_16(x) (CheckCpuEndian() ? (uint16_t)(x) : REVERSE_BYTES_ORDER_16(x))
#define NTOH_32(x) (CheckCpuEndian() ? (uint32_t)(x) : REVERSE_BYTES_ORDER_32(x))
#define NTOH_64(x) (CheckCpuEndian() ? (uint64_t)(x) : REVERSE_BYTES_ORDER_64(x))


/**
   基础类型的序列化和反序列化函数
   将每一种类型分开来写，防止没有定义Encode函数的Struct被编码
*/

inline bool Encode(StreamWriter & stream_writer, char v)
{
	return stream_writer.Write((const void *)&v, sizeof(char));
}

inline bool Decode(StreamReader & stream_reader, char & v)
{
	return stream_reader.Read((void*)&v, sizeof(char));
}

inline bool Encode(StreamWriter & stream_writer, int8_t v)
{
	return stream_writer.Write((const void *)&v, sizeof(int8_t));
}

inline bool Decode(StreamReader & stream_reader, int8_t & v)
{
	return stream_reader.Read((void*)&v, sizeof(int8_t));
}

inline bool Encode(StreamWriter & stream_writer, uint8_t v)
{
	return stream_writer.Write((const void *)&v, sizeof(uint8_t));
}

inline bool Decode(StreamReader & stream_reader, uint8_t & v)
{
	return stream_reader.Read((void*)&v, sizeof(uint8_t));
}

inline bool Encode(StreamWriter & stream_writer, int16_t v)
{
	v = (int16_t)HTON_16(v);
	return stream_writer.Write((const void *)&v, sizeof(int16_t));
}

inline bool Decode(StreamReader & stream_reader, int16_t & v)
{
	if (!stream_reader.Read((void*)&v, sizeof(int16_t)))
	{
		return false;
	}
	v = (int16_t)NTOH_16(v);
	return true;
}

inline bool Encode(StreamWriter & stream_writer, uint16_t v)
{
	v = (uint16_t)HTON_16(v);
	return stream_writer.Write((const void *)&v, sizeof(uint16_t));
}

inline bool Decode(StreamReader & stream_reader, uint16_t & v)
{
	if (!stream_reader.Read((void*)&v, sizeof(uint16_t)))
	{
		return false;
	}
	v = (uint16_t)NTOH_16(v);
	return true;
}

inline bool Encode(StreamWriter & stream_writer, int32_t v)
{
	v = (int32_t)HTON_32(v);
	return stream_writer.Write((const void *)&v, sizeof(int32_t));
}

inline bool Decode(StreamReader & stream_reader, int32_t & v)
{
	if (!stream_reader.Read((void*)&v, sizeof(int32_t)))
	{
		return false;
	}
	v = (int32_t)NTOH_32(v);
	return true;
}

inline bool Encode(StreamWriter & stream_writer, uint32_t v)
{
	v = (uint32_t)HTON_32(v);
	return stream_writer.Write((const void *)&v, sizeof(uint32_t));
}

inline bool Decode(StreamReader & stream_reader, uint32_t & v)
{
	if (!stream_reader.Read((void*)&v, sizeof(uint32_t)))
	{
		return false;
	}
	v = (uint32_t)NTOH_32(v);
	return true;
}

inline bool Encode(StreamWriter & stream_writer, int64_t v)
{
	v = (int64_t)HTON_64(v);
	return stream_writer.Write((const void *)&v, sizeof(int64_t));
}

inline bool Decode(StreamReader & stream_reader, int64_t & v)
{
	if (!stream_reader.Read((void*)&v, sizeof(int64_t)))
	{
		return false;
	}
	v = (int64_t)NTOH_64(v);
	return true;
}

inline bool Encode(StreamWriter & stream_writer, uint64_t v)
{
	v = (uint64_t)HTON_64(v);
	return stream_writer.Write((const void *)&v, sizeof(uint64_t));
}

inline bool Decode(StreamReader & stream_reader, uint64_t & v)
{
	if (!stream_reader.Read((void*)&v, sizeof(uint64_t)))
	{
		return false;
	}
	v = (uint64_t)NTOH_64(v);
	return true;
}

template<typename T>
inline int32_t GetSize(const T & v)
{
	static_assert(std::is_pod<T>::value, "GetSize type not be pod type");
	return sizeof(v);
}

inline bool Encode(StreamWriter & stream_writer, const std::string & v)
{
	uint16_t len = (uint16_t)HTON_16(v.length());

	if (!stream_writer.Write((const void *)&len, sizeof(len)))
	{
		return false;
	}

	return len > 0 ? stream_writer.Write((const void *)v.c_str(), (uint32_t)v.length()) : true;
}

inline bool Decode(StreamReader & stream_reader, std::string & v)
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

inline int32_t GetSize(const std::string & v)
{
	return sizeof(uint16_t) + (int32_t)v.length();
}

template<typename T, int Array_Size>
inline bool Encode(StreamWriter & stream_writer, const T(&v)[Array_Size])
{
	for (int i = 0; i < Array_Size; i++)
	{
		if (!Encode(stream_writer, v[i]))
		{
			return false;
		}
	}

	return true;
}

template<typename T, int Array_Size>
inline bool Decode(StreamReader & stream_reader, T(&v)[Array_Size])
{
	for (int i = 0; i < Array_Size; i++)
	{
		if (!Decode(stream_reader, v[i]))
		{
			return false;
		}
	}

	return true;
}

template<typename T, int Array_Size>
inline int32_t GetSize(const T(&v)[Array_Size])
{
	int32_t len = 0;

	for (int i = 0; i < Array_Size; i++)
	{
		len += GetSize(v[i]);
	}

	return len;
}

template<typename T>
inline bool Encode(StreamWriter & stream_writer, const std::vector<T> & v)
{
	uint16_t len = (uint16_t)HTON_16(v.size());

	if (!stream_writer.Write((const void *)&len, sizeof(len)))
	{
		return false;
	}

	for (const T & item : v)
	{
		if (!Encode(stream_writer, item))
		{
			return false;
		}
	}

	return true;
}

template<typename T>
inline bool Decode(StreamReader & stream_reader, std::vector<T> & v)
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

		if (!Decode(stream_reader, item))
		{
			return false;
		}

		v.push_back(item);
	}

	return true;
}

template<typename T>
inline int32_t GetSize(const std::vector<T> & v)
{
	int32_t len = sizeof(uint16_t);

	for (const T & item : v)
	{
		len += GetSize(item);
	}

	return len;
}

template<typename T>
inline bool Encode(StreamWriter & stream_writer, const std::list<T> & v)
{
	uint16_t len = (uint16_t)HTON_16(v.size());

	if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
	{
		return false;
	}

	for (const T & item : v)
	{
		if (!Encode(stream_writer, item))
		{
			return false;
		}
	}

	return true;
}

template<typename T>
inline bool Decode(StreamReader & stream_reader, std::list<T> & v)
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

		if (!Decode(stream_reader, item))
		{
			return false;
		}

		v.push_back(item);
	}

	return true;
}

template<typename T>
inline int32_t GetSize(const std::list<T> & v)
{
	int32_t len = sizeof(uint16_t);

	for (const T & item : v)
	{
		len += GetSize(item);
	}

	return len;
}


template<typename T>
inline bool Encode(StreamWriter & stream_writer, const std::set<T> & v)
{
	uint16_t len = (uint16_t)HTON_16(v.size());

	if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
	{
		return false;
	}

	for (const T & item : v)
	{
		if (!Encode(stream_writer, item))
		{
			return false;
		}
	}

	return true;
}

template<typename T>
inline bool Decode(StreamReader & stream_reader, std::set<T> & v)
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

		if (!Decode(stream_reader, item))
		{
			return false;
		}

		v.insert(item);
	}

	return true;
}

template<typename T>
inline int32_t GetSize(const std::set<T> & v)
{
	int32_t len = sizeof(uint16_t);

	for (const T & item : v)
	{
		len += GetSize(item);
	}

	return len;
}

template<typename T>
inline bool Encode(StreamWriter & stream_writer, const std::unordered_set<T> & v)
{
	uint16_t len = (uint16_t)HTON_16(v.size());

	if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
	{
		return false;
	}

	for (const T & item : v)
	{
		if (!Encode(stream_writer, item))
		{
			return false;
		}
	}

	return true;
}

template<typename T>
inline bool Decode(StreamReader & stream_reader, std::unordered_set<T> & v)
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

		if (!Decode(stream_reader, item))
		{
			return false;
		}

		v.insert(item);
	}

	return true;
}

template<typename T>
inline int32_t GetSize(const std::unordered_set<T> & v)
{
	int32_t len = sizeof(uint16_t);

	for (const T & item : v)
	{
		len += GetSize(item);
	}

	return len;
}


template<typename T_Key, typename T_Val>
inline bool Encode(StreamWriter & stream_writer, const std::map<T_Key, T_Val> & v)
{
	uint16_t len = (uint16_t)HTON_16(v.size());

	if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
	{
		return false;
	}

	for (const auto it : v)
	{
		if (!Encode(stream_writer, it.first) || !Encode(stream_writer, it.second))
		{
			return false;
		}
	}

	return true;
}

template<typename T_Key, typename T_Val>
inline bool Decode(StreamReader & stream_reader, std::map<T_Key, T_Val> & v)
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

		if (!Decode(stream_reader, key) || !Decode(stream_reader, val))
		{
			return false;
		}

		v.insert(std::make_pair(key, val));
	}

	return true;
}

template<typename T_Key, typename T_Val>
inline int32_t GetSize(const std::map<T_Key, T_Val> & v)
{
	int32_t len = sizeof(uint16_t);

	for (const auto it : v)
	{
		len += GetSize(it.first);
		len += GetSize(it.second);
	}

	return len;
}

template<typename T_Key, typename T_Val>
inline bool Encode(StreamWriter & stream_writer, const std::unordered_map<T_Key, T_Val> & v)
{
	uint16_t len = (uint16_t)HTON_16(v.size());

	if (!stream_writer.Write((const void *)&len, sizeof(uint16_t)))
	{
		return false;
	}

	for (const auto & it : v)
	{
		if (!Encode(stream_writer, it.first) || !Encode(stream_writer, it.second))
		{
			return false;
		}
	}

	return true;
}

template<typename T_Key, typename T_Val>
inline bool Decode(StreamReader & stream_reader, std::unordered_map<T_Key, T_Val> & v)
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

		if (!Decode(stream_reader, key) || !Decode(stream_reader, val))
		{
			return false;
		}

		v.insert(std::make_pair(key, val));
	}

	return true;
}

template<typename T_Key, typename T_Val>
inline int32_t GetSize(const std::unordered_map<T_Key, T_Val> & v)
{
	int32_t len = sizeof(uint16_t);

	for (const auto it : v)
	{
		len += GetSize(it.first);
		len += GetSize(it.second);
	}

	return len;
}

template<typename T>
inline bool Encode(StreamWriter & stream_writer, const std::shared_ptr<T> & v)
{
	uint8_t flag = v ? 1 : 0;

	if (!stream_writer.Write((const void *)&flag, sizeof(uint8_t)))
	{
		return false;
	}

	return flag == 0 ? true : Encode(stream_writer, *(v.get()));
}

template<typename T>
inline bool Decode(StreamReader & stream_reader, std::shared_ptr<T> & v)
{
	uint8_t flag;
	if (!stream_reader.Read((void*)&flag, sizeof(uint8_t)))
	{
		return false;
	}

	v.reset();

	if (flag > 0)
	{
		v = std::make_shared<T>();
		return Decode(stream_reader, *(v.get()));
	}

	return true;
}

template<typename T>
inline int32_t GetSize(const std::shared_ptr<T> & v)
{
	return sizeof(uint8_t) + GetSize(*(v.get()));
}

inline bool AutoEncode(StreamWriter & stream_writer)
{
	return true;
}

template<typename T>
inline bool AutoEncode(StreamWriter & stream_writer, const T & t)
{
	return Encode(stream_writer, t);
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
	return Decode(stream_reader, t);
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
	return GetSize(t);
}

template<typename T, typename... T_Args>
inline int32_t AutoGetSize(const T & t, const T_Args&... args)
{
	return AutoGetSize<T>(t) + AutoGetSize<T_Args...>(args...);
}

// 自定义协议
#define SERIALIZABLE_STRUCT(S) \
	struct S; \
	int32_t GetSize(const S & obj); \
	bool Encode(StreamWriter & stream_writer, const S & obj); \
	bool Decode(StreamReader & stream_reader, S & obj); \
	struct S

// 自定义协议编解码包装宏
#define SERIALIZE(S, ...) \
	int32_t GetSize(const S & obj) { return AutoGetSize(__VA_ARGS__); } \
	bool Encode(StreamWriter & stream_writer, const S & obj) { return AutoEncode(stream_writer, ##__VA_ARGS__); } \
	bool Decode(StreamReader & stream_reader, S & obj) { return AutoDecode(stream_reader, ##__VA_ARGS__);}

#define SERIALIZE1(S, v1) \
	SERIALIZE(S, obj.v1)

#define SERIALIZE2(S, v1, v2) \
	SERIALIZE(S, obj.v1, obj.v2)

#define SERIALIZE3(S, v1, v2, v3) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3)

#define SERIALIZE4(S, v1, v2, v3, v4) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4)

#define SERIALIZE5(S, v1, v2, v3, v4, v5) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5)

#define SERIALIZE6(S, v1, v2, v3, v4, v5, v6) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6)

#define SERIALIZE7(S, v1, v2, v3, v4, v5, v6, v7) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7)

#define SERIALIZE8(S, v1, v2, v3, v4, v5, v6, v7, v8) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8)

#define SERIALIZE9(S, v1, v2, v3, v4, v5, v6, v7, v8, v9) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9)

#define SERIALIZE10(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10)

#define SERIALIZE11(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11)

#define SERIALIZE12(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12)

#define SERIALIZE13(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13)

#define SERIALIZE14(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14)

#define SERIALIZE15(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14, obj.v15)

#define SERIALIZE16(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14, obj.v15, obj.v16)

#define SERIALIZE17(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14, obj.v15, obj.v16, obj.v17)

#define SERIALIZE18(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14, obj.v15, obj.v16, obj.v17, obj.v18)

#define SERIALIZE19(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14, obj.v15, obj.v16, obj.v17, obj.v18, obj.v19)

#define SERIALIZE20(S, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) \
	SERIALIZE(S, obj.v1, obj.v2, obj.v3, obj.v4, obj.v5, obj.v6, obj.v7, obj.v8, obj.v9, obj.v10, obj.v11, obj.v12, obj.v13, obj.v14, obj.v15, obj.v16, obj.v17, obj.v18, obj.v19, obj.v20)

#endif