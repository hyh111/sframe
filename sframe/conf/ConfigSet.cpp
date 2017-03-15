
#include <memory.h>
#include <assert.h>
#include "ConfigSet.h"

using namespace sframe;


// 加载(全部成功返回true, 只要有一个失败都会返回false)
// 出错时，err_info会返回出错的配置信息
bool ConfigSet::Load(const std::string & path, std::vector<ConfigError> * vec_err_info)
{
	if (path.empty() || !_config.empty())
	{
		return false;
	}

	_config_dir = path;

	std::map<int32_t, std::shared_ptr<ConfigBase>> map_conf;
	bool ret = true;

	for (auto & pr : _config_load_helper)
	{
		ConfigLoadHelper & load_helper = pr.second;
		auto conf = LoadAndInitConfig(load_helper, vec_err_info);
		if (!conf)
		{
			ret = false;
			continue;
		}

		_config[load_helper.conf_id] = conf;
	}

	for (auto & load_helper : _temporary_config_load_helper)
	{
		auto conf = LoadAndInitConfig(load_helper, vec_err_info);
		if (!conf)
		{
			ret = false;
			continue;
		}
	}

	return ret;
}

// 加载并初始化一个配置
std::shared_ptr<ConfigSet::ConfigBase> ConfigSet::LoadAndInitConfig(const ConfigLoadHelper & load_helper, std::vector<ConfigError> * err_info)
{
	// 加载
	auto conf = (this->*load_helper.func_load)(load_helper.conf_file_name);
	if (!conf)
	{
		if (err_info)
		{
			ConfigError err;
			err.err_type = ConfigError::kLoadConfigError;
			err.config_type = load_helper.conf_id;
			err.config_file_name = load_helper.conf_file_name;
			err_info->push_back(err);
		}

		return NULL;
	}

	// 初始化
	if (!(this->*load_helper.func_init)(conf))
	{
		if (err_info)
		{
			ConfigError err;
			err.err_type = ConfigError::kInitConfigError;
			err.config_type = load_helper.conf_id;
			err.config_file_name = load_helper.conf_file_name;
			err_info->push_back(err);
		}

		return NULL;
	}

	return conf;
}