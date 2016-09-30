
#ifdef __GNUC__
#include <cxxabi.h>
#endif

#include <stdlib.h>
#include "ObjectFactory.h"

#ifndef __GNUC__

// 解析类型名称（转换为 A::B::C 的形式）
std::string sframe::ReadTypeName(const char * name)
{
	const char * p = strstr(name, " ");
	if (p)
	{
		size_t prev_len = (size_t)(p - name);
		if (memcmp(name, "class", prev_len) == 0 ||
			memcmp(name, "struct", prev_len) == 0 ||
			memcmp(name, "enum", prev_len) == 0 ||
			memcmp(name, "union", prev_len) == 0)
		{
			p += 1;
			return std::string(p);
		}
	}

	return std::string(name);
}

#else
	
// 解析类型名称（转换为 A::B::C 的形式）
std::string sframe::ReadTypeName(const char * name)
{
	char * real_name = abi::__cxa_demangle(name, nullptr, nullptr, nullptr);
	std::string real_name_string(real_name);
	free(real_name);
	return real_name_string;
}

#endif
