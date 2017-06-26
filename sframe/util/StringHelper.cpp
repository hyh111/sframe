
#include <string.h>
#include <algorithm>
#include <assert.h>
#include <locale>
#include "StringHelper.h"

// 分割字符串
std::vector<std::string> sframe::SplitString(const std::string & str, const std::string & sep)
{
	std::vector<std::string> result;

	if (str.size() == 0)
	{
		return result;
	}

	size_t oldPos, newPos;
	oldPos = 0;
	newPos = 0;

	while (1)
	{
		newPos = str.find(sep, oldPos);
		if (newPos != std::string::npos)
		{
			result.push_back(std::move(str.substr(oldPos, newPos - oldPos)));
			oldPos = newPos + sep.size();
		}
		else if (oldPos <= str.size())
		{
			result.push_back(std::move(str.substr(oldPos)));
			break;
		}
		else
		{
			break;
		}
	}

	return result;
}

// 查找字符在字符串中最大连续出现次数
int32_t sframe::GetCharMaxContinueInString(const std::string & str, char c)
{
	int32_t cur = 0;
	int32_t max = 0;

	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == c)
		{
			cur++;
		}
		else
		{
			max = std::max(max, cur);
			cur = 0;
		}
	}

	max = std::max(max, cur);

	return max;
}

// 查找子串
int32_t sframe::FindFirstSubstr(const char * str, int32_t len, const char * sub_str)
{
	int32_t sub_str_len = (int32_t)strlen(sub_str);
	if (sub_str_len <= 0 || len < sub_str_len)
	{
		return -1;
	}

	for (int32_t i = 0; i < len - sub_str_len + 1; i++)
	{
		if (memcmp(str + i, sub_str, sub_str_len) == 0)
		{
			return i;
		}
	}

	return -1;
}


static const char kUpperLower = 'a' - 'Z';

// 将字符串转换为大写
void sframe::UpperString(std::string & str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		char & c = (*it);
		if (c >= 'a' && c <= 'z')
		{
			c -= kUpperLower;
		}
	}
}

// 将字符串转换为小写
void sframe::LowerString(std::string & str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		char & c = (*it);
		if (c >= 'A' && c <= 'Z')
		{
			c += kUpperLower;
		}
	}
}

// 将字符串转换为大写
std::string sframe::ToUpper(const std::string & str)
{
	std::string new_str;
	new_str.reserve(str.size());

	for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
	{
		char c = (*it);
		if (c >= 'a' && c <= 'z')
		{
			c -= kUpperLower;
		}
		new_str.push_back(c);
	}

	return new_str;
}

// 将字符串转换为小写
std::string sframe::ToLower(const std::string & str)
{
	std::string new_str;
	new_str.reserve(str.size());

	for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
	{
		char c = (*it);
		if (c >= 'A' && c <= 'Z')
		{
			c += kUpperLower;
		}
		new_str.push_back(c);
	}

	return new_str;
}

// 去掉头部空串
std::string sframe::TrimLeft(const std::string & str)
{
	size_t pos = str.find_first_not_of(' ');
	if (pos == std::string::npos)
	{
		return "";
	}

	return str.substr(pos);
}

// 去掉尾部空串
std::string sframe::TrimRight(const std::string & str)
{
	size_t pos = str.find_last_not_of(' ');
	if (pos == std::string::npos)
	{
		return "";
	}

	return str.substr(0, pos + 1);
}

// 去掉两边空串
std::string sframe::Trim(const std::string & str)
{
	return TrimLeft(TrimRight(str));
}

// wstring -> string
std::string sframe::WStrToStr(const std::wstring& src)
{
	const wchar_t* data_from = src.c_str();
	const wchar_t* data_from_end = src.c_str() + src.size();
	const wchar_t* data_from_next = 0;

	size_t buf_size = (src.size() + 1) * sizeof(wchar_t);
	std::vector<char> buf(buf_size);
	char* data_to = &(*buf.begin());
	char* data_to_end = data_to + buf_size;
	char* data_to_next = nullptr;

	typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
	mbstate_t out_state = { 0 };
	std::locale sys_locale("");
	auto result = std::use_facet<convert_facet>(sys_locale).out(
		out_state, data_from, data_from_end, data_from_next, data_to, data_to_end, data_to_next);
	if (result != convert_facet::ok)
	{
		return std::string();
	}

	return std::string(data_to);
}

// string -> wstring
std::wstring sframe::StrToWStr(const std::string& src)
{
	const char* data_from = src.c_str();
	const char* data_from_end = src.c_str() + src.size();
	const char* data_from_next = 0;

	size_t buf_size = src.size() + 1;
	std::vector<wchar_t> buf(buf_size);
	wchar_t* data_to = &(*buf.begin());
	wchar_t* data_to_end = data_to + buf_size;
	wchar_t* data_to_next = 0;

	typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
	mbstate_t in_state = { 0 };
	std::locale sys_locale("");
	auto result = std::use_facet<convert_facet>(sys_locale).in(
		in_state, data_from, data_from_end, data_from_next, data_to, data_to_end, data_to_next);
	if (result != convert_facet::ok)
	{
		return std::wstring();
	}

	return std::wstring(data_to);
}

// utf8 -> wchar
size_t sframe::UTF8ToWChar(const char * str, size_t len, wchar_t * wc)
{
	if (wc)
	{
		(*wc) = (wchar_t)0;
	}

	if (str == nullptr || len == 0)
	{
		return 0;
	}

	uint32_t chr_data = 0;
	const uint8_t * data = (const uint8_t *)str;
	size_t bytes_num = 0;
	// 1字节 [0, 0x80)
	if (data[0] < 0x80)
	{
		chr_data = data[0];
		bytes_num = 1;
	}
	// 2个字节 [0xc0, 0xe0)
	else if (data[0] >= 0xc0 && data[0] < 0xe0)
	{
		chr_data = (data[0] & 0x1f);
		bytes_num = 2;
	}
	// 3字节 [0xe0, 0xf0)
	else if (data[0] < 0xf0)
	{
		chr_data = (data[0] & 0x0f);
		bytes_num = 3;
	}
	// 4字节 [0xf0, 0xf8)
	else if (data[0] < 0xf8)
	{
		chr_data = (data[0] & 0x07);
		bytes_num = 4;
	}
	// 5字节 [0xf8, 0xfc)
	else if (data[0] < 0xfc)
	{
		chr_data = (data[0] & 0x03);
		bytes_num = 5;
	}
	// 6字节 [0xfc, 0xfe)
	else if (data[0] < 0xfe)
	{
		chr_data = (data[0] & 0x01);
		bytes_num = 6;
	}
	else
	{
		return 0;
	}

	assert(bytes_num >= 1 && bytes_num <= 6);
	if (bytes_num > len)
	{
		return 0;
	}

	for (size_t i = 1; i < bytes_num; i++)
	{
		if (data[i] < 0x80 || data[i] > 0xbf)
		{
			return 0;
		}

		chr_data = ((chr_data << 6) & 0xffffffc0) + (data[i] & 0x3f);
	}

	if (wc && (sizeof(wchar_t) >= 4 || (sizeof(wchar_t) >= 2 && bytes_num <= 3)))
	{
		(*wc) = (wchar_t)chr_data;
	}

	return bytes_num;
}

// wchar -> utf8
size_t sframe::WCharToUTF8(wchar_t wc, char * buf, size_t buf_size)
{
	uint32_t bin = (uint32_t)wc;
	size_t bytes_num = 0;

	if (bin < 0x80)
	{
		bytes_num = 1;
	}
	else if (bin < 0x800)
	{
		bytes_num = 2;
	}
	else if (bin < 0x10000)
	{
		bytes_num = 3;
	}
	else if (bin < 0x200000)
	{
		bytes_num = 4;
	}
	else if (bin < 0x4000000)
	{
		bytes_num = 5;
	}
	else if (bin < 0x80000000)
	{
		bytes_num = 6;
	}
	else
	{
		return 0;
	}

	if (buf == nullptr || buf_size < bytes_num)
	{
		return 0;
	}

	for (size_t i = bytes_num - 1; i > 0 ; i--)
	{
		buf[i] = (char)((bin & 0x0000003f) | 0x00000080);
		bin >>= 6;
	}

	static const uint8_t kBytesNumToPrefix[7] = { 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc };
	buf[0] = (char)((uint8_t)bin | kBytesNumToPrefix[bytes_num]);

	return bytes_num;
}

// 是否合法的utf8
bool sframe::IsValidUTF8(const std::string & str)
{
	return IsValidUTF8(str.data(), str.length());
}

// 是否合法的utf8
bool sframe::IsValidUTF8(const char * str, size_t len)
{
	while (len > 0)
	{
		size_t cur_size = UTF8ToWChar(str, len, nullptr);
		assert(cur_size <= len);
		if (cur_size == 0)
		{
			return false;
		}
		str += cur_size;
		len -= cur_size;
	}
	return true;
}

// wstring -> utf8
std::string sframe::WStrToUTF8(const std::wstring& src)
{
	std::string s;

	for (wchar_t c : src)
	{
		char buf[6];
		size_t bytes_num = WCharToUTF8(c, buf, sizeof(buf));
		if (bytes_num == 0)
		{
			return "";
		}

		s.append(buf, bytes_num);
	}

	return s;
}

// utf8 -> wstring
std::wstring sframe::UTF8ToWStr(const std::string& src)
{
	const char * str = src.data();
	size_t len = src.length();
	std::wstring wstr;
	wstr.reserve(src.size());

	while (len > 0)
	{
		wchar_t wchr = (wchar_t)0;
		size_t bytes_num = UTF8ToWChar(str, len, &wchr);
		assert(bytes_num <= len);
		if (bytes_num == 0 || wchr == 0)
		{
			return L"";
		}
		wstr.push_back(wchr);
		str += bytes_num;
		len -= bytes_num;
	}

	return wstr;
}