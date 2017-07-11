
#include <sstream>
#include "ConfigManager.h"
#include "conf/csv.h"
#include "conf/TableLoader.h"
#include "conf/JsonLoader.h"
#include "util/Log.h"

using namespace sframe;

void ConfigManager::RegistAllConfig(ConfigSet & conf_set)
{
	conf_set.RegistConfig<TableLoader<CSV>, ClientConfig>("client_config.csv");
}




std::shared_ptr<ConfigSet> ConfigManager::g_cur_conf_set;
std::string ConfigManager::g_config_path;
sframe::Lock ConfigManager::g_conf_set_lock;

// 初始化
bool ConfigManager::InitializeConfig(const std::string & path)
{
	g_config_path = path;
	if (g_config_path.empty() || *(g_config_path.end() - 1) != '/')
	{
		g_config_path.push_back('/');
	}

	return ReloadConfig();
}

// 重新加载
bool ConfigManager::ReloadConfig()
{
	if (g_config_path.empty())
	{
		assert(false);
		return false;
	}

	std::shared_ptr<ConfigSet> conf_set = std::make_shared<ConfigSet>();
	if (!conf_set)
	{
		assert(false);
		return false;
	}

	RegistAllConfig(*conf_set);
	std::vector<std::string> vec_err_msg;
	if (!conf_set->Load(g_config_path, &vec_err_msg))
	{
		for (const std::string err : vec_err_msg)
		{
			LOG_ERROR << err << std::endl;
		}

		return false;
	}

	AUTO_LOCK(g_conf_set_lock);
	g_cur_conf_set = conf_set;

	return true;
}

// 获取配置集
std::shared_ptr<ConfigSet> ConfigManager::GetConfigSet()
{
	AUTO_LOCK(g_conf_set_lock);
	assert(g_cur_conf_set);
	return g_cur_conf_set;
}