
#include <memory.h>
#include <assert.h>
#include <sstream>
#include "ConfigSet.h"

using namespace sframe;


// 加载(全部成功返回true, 只要有一个失败都会返回false)
// 出错时，err_info会返回出错的配置信息
bool ConfigSet::Load(const std::string & path, std::vector<std::string> * vec_err_msg)
{
	if (path.empty() || !_config.empty())
	{
		return false;
	}

	_config_dir = path;

	std::map<int32_t, std::shared_ptr<ConfigSet::ConfigBase>> map_conf;
	bool ret = true;

	for (auto & pr : _config_load_helper)
	{
		ConfigLoadHelper & load_helper = pr.second;
		auto conf = LoadAndInitConfig(load_helper, vec_err_msg);
		if (!conf)
		{
			ret = false;
			continue;
		}

		_config[load_helper.conf_id] = conf;
	}

	for (auto & load_helper : _temporary_config_load_helper)
	{
		auto conf = LoadAndInitConfig(load_helper, vec_err_msg);
		if (!conf)
		{
			ret = false;
			continue;
		}
	}

	return ret;
}

// 加载并初始化一个配置
std::shared_ptr<ConfigSet::ConfigBase> ConfigSet::LoadAndInitConfig(const ConfigLoadHelper & load_helper, std::vector<std::string> * vec_err_msg)
{
	std::vector<std::string> err_files;

	// 加载
	auto conf = (this->*load_helper.func_load)(load_helper.conf_file_name, vec_err_msg ? &err_files : nullptr);
	if (!conf)
	{
		if (vec_err_msg)
		{
			for (const std::string & f : err_files)
			{
				std::ostringstream oss;
				oss << "Load config error, config type(" << load_helper.conf_type_name << "), config file(" << f << ')';
				vec_err_msg->push_back(oss.str());
			}
		}

		return NULL;
	}

	// 初始化
	if (!(this->*load_helper.func_init)(conf))
	{
		if (vec_err_msg)
		{
			std::ostringstream oss;
			oss << "Initialize config error, config type(" << load_helper.conf_type_name << ')';
			vec_err_msg->push_back(oss.str());
		}

		return NULL;
	}

	return conf;
}