
#ifndef SFRAME_CONVERT_H
#define SFRAME_CONVERT_H

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string>

namespace sframe {

template<typename R>
inline R StrToAny(const std::string & str)
{
	assert(false);
	return R();
}


template<>
inline std::string StrToAny<std::string>(const std::string & str)
{
	return str;
}

template<>
inline int8_t StrToAny<int8_t>(const std::string & str)
{
	return (int8_t)strtol(str.c_str(), nullptr, 10);
}

template<>
inline uint8_t StrToAny<uint8_t>(const std::string & str)
{
	return (uint8_t)strtoul(str.c_str(), nullptr, 10);
}

template<>
inline int16_t StrToAny<int16_t>(const std::string & str)
{
	return (int16_t)strtol(str.c_str(), nullptr, 10);
}

template<>
inline uint16_t StrToAny<uint16_t>(const std::string & str)
{
	return (uint16_t)strtoul(str.c_str(), nullptr, 10);
}

template<>
inline int32_t StrToAny<int32_t>(const std::string & str)
{
	return (int32_t)strtol(str.c_str(), nullptr, 10);
}

template<>
inline uint32_t StrToAny<uint32_t>(const std::string & str)
{
	return (uint32_t)strtoul(str.c_str(), nullptr, 10);
}

template<>
inline int64_t StrToAny<int64_t>(const std::string & str)
{
	return strtoll(str.c_str(), nullptr, 10);
}

template<>
inline uint64_t StrToAny<uint64_t>(const std::string & str)
{
	return strtoull(str.c_str(), nullptr, 10);
}

template<>
inline double StrToAny<double>(const std::string & str)
{
	return strtod(str.c_str(), nullptr);
}

template<>
inline float StrToAny<float>(const std::string & str)
{
	return strtof(str.c_str(), nullptr);
}

}

#endif