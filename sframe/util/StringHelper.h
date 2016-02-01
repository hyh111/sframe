
#ifndef SFRAME_STRING_HELPER_H
#define SFRAME_STRING_HELPER_H

#include <string>
#include <vector>

namespace sframe {

// 分割字符串
int32_t SplitString(const std::string & str, std::vector<std::string> & result, const std::string & sep);

// 查找字符在字符串中最大连续出现次数
int32_t GetCharMaxContinueInString(const std::string & str, char c);

}

#endif
