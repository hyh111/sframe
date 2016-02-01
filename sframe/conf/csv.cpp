
#include <sstream>
#include "csv.h"
#include "../util/StringHelper.h"

using namespace sframe;

// 解析
bool CSV::Parse(const std::string & content, Table & tbl)
{
	if (content.empty())
	{
		return true;
	}

	Row * cur_row = nullptr;
	int32_t cur_column = 0;
	size_t i = 0;
	std::string word;
	bool word_start_by_quote = false;    // 当前词语是否已引号开始
	bool word_first = true;              // 当前是否是一个词的第一个字符

	while (true)
	{
		char c = content[i];
		if (c == '\0')
		{
			if (!word.empty())
			{
				if (cur_row == nullptr)
				{
					cur_column = 0;
					cur_row = &tbl.NewRow();
				}
				*cur_row->GetValue(cur_column++, true) = word;
				word.clear();
			}
			break;
		}

		if (word_first)
		{
			word_first = false;
			if (c == '"')
			{
				word_start_by_quote = true;
				i++;
				continue;
			}
		}

		if (word_start_by_quote)
		{
			if (c == '"')
			{
				i++;
				if (content[i] == '"')
				{
					word.push_back('"');
					i++;
				}
				else
				{
					word_start_by_quote = false;
				}
				continue;
			}
		}
		else if (c == ',' || c == '\n' || (c == '\r' && content[i + 1] == '\n'))
		{
			if (cur_row == nullptr)
			{
				cur_column = 0;
				cur_row = &tbl.NewRow();
			}
			*cur_row->GetValue(cur_column++, true) = word;
			word.clear();
			word_first = true;
			i++;
			if (c == '\r' && content[i] == '\n')
			{
				cur_row = nullptr;
				i++;
			}
			else if (c == '\n')
			{
				cur_row = nullptr;
			}

			continue;
		}

		word.push_back(c);
		i++;
	}

	return true;
}
