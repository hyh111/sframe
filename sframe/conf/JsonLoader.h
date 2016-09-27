
#ifndef SFRAME_JSON_LOADER_H
#define SFRAME_JSON_LOADER_H

#include "../util/FileHelper.h"
#include "../util/Log.h"
#include "JsonReader.h"

namespace sframe {

// JSON¶ÁÈ¡Æ÷
class JsonLoader
{
public:
	template<typename T>
	static bool Load(const std::string & full_name, T & o)
	{
		std::string content;
		if (!FileHelper::ReadFile(full_name, content))
		{
			LOG_ERROR << "Cannot open config file(" << full_name << ")" << ENDL;
			return false;
		}

		std::string err;
		json11::Json json = json11::Json::parse(content, err);
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