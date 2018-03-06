
#ifndef SFRAME_JSON_LOADER_H
#define SFRAME_JSON_LOADER_H

#include <sstream>
#include "../util/FileHelper.h"
#include "../util/Log.h"
#include "JsonReader.h"

namespace sframe {

// JSON¶ÁÈ¡Æ÷
class JsonLoader
{
public:

	// È¥×¢ÊÍ
	static std::string RemoveComments(const std::string & data)
	{
		int cur_state = 0;   // 0 ²»ÊÇ×¢ÊÍ, 1 ÐÐ×¢ÊÍ, 2¶Î×¢ÊÍ 
		std::ostringstream oss;
		auto it = data.begin();
		while (it < data.end())
		{
			char c = *it;
			if (cur_state == 1)
			{
				if (c == '\n')
				{
					// ÐÐ×¢ÊÍ½áÊø
					cur_state = 0;
				}
			}
			else if (cur_state == 2)
			{
				if (c == '*' &&  it < data.end() - 1 && (*(it + 1)) == '/')
				{
					it++;
					// ¶Î×¢ÊÍ½áÊø
					cur_state = 0;
				}
			}
			else
			{
				if (c == '/' && it < data.end() - 1)
				{
					char next = *(it + 1);
					if (next == '/')
					{
						cur_state = 1;
						it += 2;
						continue;
					}
					else if (next == '*')
					{
						cur_state = 2;
						it += 2;
						continue;
					}
				}

				oss << c;
			}

			it++;
		}

		return oss.str();
	}

	template<typename T>
	static bool Load(const std::string & full_name, T & o)
	{
		std::string content;
		if (!FileHelper::ReadFile(full_name, content))
		{
			LOG_ERROR << "Cannot open config file(" << full_name << ")" << ENDL;
			return false;
		}

		std::string real_content = RemoveComments(content);

		std::string err;
		json11::Json json = json11::Json::parse(real_content, err);
		if (!err.empty())
		{
			LOG_ERROR << "Cannot parse json file(" << full_name << ") : " << err << ENDL;
			return false;
		}

		return ConfigLoader::Load<const json11::Json>(json, o);
	}
};

}

#endif