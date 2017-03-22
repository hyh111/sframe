
#ifndef SFRAME_STRING_HELPER_H
#define SFRAME_STRING_HELPER_H

#include <string>
#include <vector>

namespace sframe {

// 分割字符串
std::vector<std::string> SplitString(const std::string & str, const std::string & sep);

// 查找字符在字符串中最大连续出现次数
int32_t GetCharMaxContinueInString(const std::string & str, char c);

// 查找子串
int32_t FindFirstSubstr(const char * str, int32_t len, const char * sub_str);

// 将字符串转换为大写
void UpperString(std::string & str);

// 将字符串转换为小写
void LowerString(std::string & str);

// 将字符串转换为大写
std::string ToUpper(const std::string & str);

// 将字符串转换为小写
std::string ToLower(const std::string & str);

// 去掉头部空串
std::string TrimLeft(const std::string & str);

// 去掉尾部空串
std::string TrimRight(const std::string & str);

// 去掉两边空串
std::string Trim(const std::string & str);

// wstring -> string
std::string WStrToStr(const std::wstring & src);

// string -> wstring
std::wstring StrToWStr(const std::string & src);

// utf8 -> wchar
size_t UTF8ToWChar(const char * str, size_t len, wchar_t * wc);

// wchar -> utf8
size_t WCharToUTF8(wchar_t wc, char * buf, size_t buf_size);

// 是否合法的utf8
bool IsValidUTF8(const std::string & str);

// 是否合法的utf8
bool IsValidUTF8(const char * str, size_t len);

// wstring -> utf8
std::string WStrToUTF8(const std::wstring & src);

// utf8 -> wstring
std::wstring UTF8ToWStr(const std::string & src);

}

#endif
