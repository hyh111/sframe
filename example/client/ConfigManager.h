
#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include "util/Lock.h"
#include "util/Singleton.h"
#include "conf/ConfigSet.h"
#include "ConfigType.h"
#include "ClientConfig.h"

class ConfigManager
{
public:
	
	// ³õÊ¼»¯
	static bool InitializeConfig(const std::string & path);

	// ÖØÐÂ¼ÓÔØ
	static bool ReloadConfig();

	// »ñÈ¡ÅäÖÃ¼¯
	static std::shared_ptr<sframe::ConfigSet> GetConfigSet();

private:

	static void RegistAllConfig(sframe::ConfigSet & conf_set);

	static std::shared_ptr<sframe::ConfigSet> g_cur_conf_set;

	static std::string g_config_path;

	static sframe::Lock g_conf_set_lock;
};

#endif