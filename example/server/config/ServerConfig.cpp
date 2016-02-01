
#include <algorithm>
#include "ServerConfig.h"
#include "conf/JsonReader.h"
#include "util/FileHelper.h"
#include "util/Log.h"

using namespace sframe;

FILL_JSONCONFIG(ServerInfo)
{
	JSON_FILLFIELD_INDEX(0, ip);
	JSON_FILLFIELD_INDEX(1, port);
	JSON_FILLFIELD_INDEX(2, key);
}

bool ServerConfig::Load(const std::string & filename)
{
	std::string content;
	if (!FileHelper::ReadFile(filename, content))
	{
		LOG_ERROR << "Open config file(" << filename << ") error" << ENDL;
		return false;
	}

	std::string err;
	json11::Json json = json11::Json::parse(content, err);
	if (!err.empty())
	{
		LOG_ERROR << "Parse config file(" << filename << ") error: " << err << ENDL;
		return false;
	}

	Json_FillObject(json, *this);

	return true;
}

FILL_JSONCONFIG(ServerConfig)
{
	JSON_FILLFIELD_WITH_DEFAULT(res_path, std::string("./res"));
	JSON_FILLFIELD_WITH_DEFAULT(thread_num, 2);
	JSON_FILLFIELD(local_service);
	JSON_FILLFIELD(service_listen);
	JSON_FILLFIELD(client_listen);
	JSON_FILLFIELD(remote_server);

	if (obj.res_path.empty() || *(obj.res_path.end() - 1) != '/' || *(obj.res_path.end() - 1) != '\\')
	{
		obj.res_path.push_back('/');
	}

	obj.thread_num = std::max(1, obj.thread_num);
}