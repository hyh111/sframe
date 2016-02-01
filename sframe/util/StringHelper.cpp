
#include <algorithm>
#include <assert.h>
#include "StringHelper.h"

// 分割字符串
int32_t sframe::SplitString(const std::string & str, std::vector<std::string> & result, const std::string & sep)
{
	if (str.size() == 0)
	{
		return 0;
	}

	if (sep.empty())
	{
		result.push_back(str);
		return 1;
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

	return (int32_t)result.size();
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
