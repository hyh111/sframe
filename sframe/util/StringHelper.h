
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

}

#endif
