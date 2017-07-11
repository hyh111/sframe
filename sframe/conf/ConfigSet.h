
#ifndef SFRAME_CONFIG_SET_H
#define SFRAME_CONFIG_SET_H

#include <assert.h>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <utility>
#include <typeinfo>
#include "ConfigLoader.h"
#include "ConfigMeta.h"
#include "../util/FileHelper.h"
#include "../util/StringHelper.h"

namespace sframe {

// 配置集合
class ConfigSet
{
public:

	struct ConfigBase
	{
		virtual ~ConfigBase() {}
	};

	template<typename T>
	struct Config : public ConfigBase
	{
		Config()
		{
			val = std::make_shared<T>();
		}

		virtual ~Config() {}

		std::shared_ptr<T> val;
	};

	typedef std::shared_ptr<ConfigSet::ConfigBase> (ConfigSet::*Func_LoadConfig)(const std::string &, std::vector<std::string> *);

	typedef bool (ConfigSet::*Func_InitConfig)(std::shared_ptr<ConfigSet::ConfigBase> &);

	typedef const char *(*Func_GetConfigName)();

	struct ConfigLoadHelper 
	{
		int32_t conf_id;
		Func_LoadConfig func_load;
		Func_InitConfig func_init;
		std::string conf_file_name;
		std::string conf_type_name;
	};

	ConfigSet() {}

	virtual ~ConfigSet() {}

	// 加载(全部成功返回true, 只要有一个失败都会返回false)
	// 出错时，err_info会返回出错的配置信息
	bool Load(const std::string & path, std::vector<std::string> * vec_err_msg = nullptr);


	///////////////////////// 查询相关方法 ///////////////////////////

	// 获取配置
	template<typename T>
	std::shared_ptr<const typename CONFIG_MODEL_TYPE(T)> GetConfig();

	// 根据key获取Map类型条目
	template<typename T>
	std::shared_ptr<const T> GetMapConfigItem(const typename CONFIG_KEY_TYPE(T) & key);

	// 获取动态配置
	template<typename T>
	std::shared_ptr<const typename DYNAMIC_CONFIG_MODEL_TYPE(T)> GetDynamicConfig();

	// 获取Map类型的动态配置的一个条目
	template<typename T>
	std::shared_ptr<const T> GetMapDynamicConfigItem(const typename DYNAMIC_CONFIG_KEY_TYPE(T) & key);

	/////////////////////////// 注册创建相关

	// 注册配置
	// conf_file_name：配置文件名，必须为Load时传入基础目录路劲的相对路劲，支持通配符以表示多个文件
	template<typename T_ConfigLoader, typename T>
	void RegistConfig(const std::string & conf_file_name);

	// 创建动态配置(若已存在，则返回已经存在的，类型匹配失败的话，返回nullptr)
	template<typename T>
	std::shared_ptr<typename DYNAMIC_CONFIG_MODEL_TYPE(T)> CreateDynamicConfig();

private:

	// 加载
	template<typename T_ConfigLoader, typename T>
	static bool LoadConfig(const std::string & conf_file_name, std::shared_ptr<Config<typename CONFIG_MODEL_TYPE(T)>> & o);

	// 加载（单个文件）
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigSet::ConfigBase> LoadConfig_OneFile(const std::string & conf_file_name, std::vector<std::string> * err_file_name);

	// 加载（多文件）
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigSet::ConfigBase> LoadConfig_MultiFile(const std::string & conf_file_name, std::vector<std::string> * err_file_name);

	// 初始化配置
	template<typename T_Obj, typename T_Config>
	bool InitConfig(std::shared_ptr<ConfigSet::ConfigBase> & conf);

	// 加载并初始化一个配置
	std::shared_ptr<ConfigSet::ConfigBase> LoadAndInitConfig(const ConfigLoadHelper & load_helper, std::vector<std::string> * vec_err_msg);

private:
	std::unordered_map<int32_t, std::shared_ptr<ConfigBase>> _config;
	std::unordered_map<int32_t, ConfigLoadHelper> _config_load_helper;
	std::unordered_map<int32_t, std::shared_ptr<ConfigBase>> _dynamic_config;
	std::vector<ConfigLoadHelper> _temporary_config_load_helper;
	std::string _config_dir;
};


// 获取配置
template<typename T>
std::shared_ptr<const typename CONFIG_MODEL_TYPE(T)> ConfigSet::GetConfig()
{
	int32_t config_id = GET_CONFIGID(T);
	auto it = _config.find(config_id);
	if (it == _config.end())
	{
		return nullptr;
	}

	std::shared_ptr<Config<typename CONFIG_MODEL_TYPE(T)>> config_ele =
		std::static_pointer_cast<Config<typename CONFIG_MODEL_TYPE(T)>>(it->second);

	return config_ele ? config_ele->val : nullptr;
}

// 根据key获取Map类型条目
template<typename T>
std::shared_ptr<const T> ConfigSet::GetMapConfigItem(const typename CONFIG_KEY_TYPE(T) & key)
{
	std::shared_ptr<const typename CONFIG_MODEL_TYPE(T)> map_conf = GetConfig<T>();
	if (!map_conf)
	{
		return nullptr;
	}

	auto it = map_conf->find(key);
	if (it == map_conf->end())
	{
		return nullptr;
	}

	return it->second;
}

// 获取动态配置
template<typename T>
std::shared_ptr<const typename DYNAMIC_CONFIG_MODEL_TYPE(T)> ConfigSet::GetDynamicConfig()
{
	int32_t conf_id = GET_DYNAMIC_CONFIGID(T);
	auto it = _dynamic_config.find(conf_id);
	if (it == _dynamic_config.end())
	{
		return nullptr;
	}

	std::shared_ptr<Config<typename DYNAMIC_CONFIG_MODEL_TYPE(T)>> config_ele =
		std::static_pointer_cast<Config<typename DYNAMIC_CONFIG_MODEL_TYPE(T)>>(it->second);
	if (!config_ele)
	{
		return nullptr;
	}

	return config_ele->val;
}

// 获取Map类型的动态配置的一个条目
template<typename T>
std::shared_ptr<const T> ConfigSet::GetMapDynamicConfigItem(const typename DYNAMIC_CONFIG_KEY_TYPE(T) & key)
{
	auto map_config = GetDynamicConfig<T>();
	if (!map_config)
	{
		return nullptr;
	}

	auto it = map_config->find(key);
	if (it == map_config->end())
	{
		return nullptr;
	}

	return it->second;
}

// 注册配置
// conf_file_name：配置文件名，必须为Load时传入基础目录路劲的相对路劲，支持通配符以表示多个文件
template<typename T_ConfigLoader, typename T>
void ConfigSet::RegistConfig(const std::string & conf_file_name)
{
	if (conf_file_name.empty())
	{
		assert(false);
		return;
	}

	int32_t config_id = GET_CONFIGID(T);
	ConfigLoadHelper load_helper;
	load_helper.conf_id = config_id;
	load_helper.func_init = &ConfigSet::InitConfig<T, typename CONFIG_MODEL_TYPE(T)>;
	load_helper.conf_file_name = conf_file_name;
	load_helper.conf_type_name = ReadTypeName(typeid(T).name());

	// 看文件命中是否有通配符(*)，若有的话，采用多文件加载方式
	if (load_helper.conf_file_name.find('*') == std::string::npos)
	{
		load_helper.func_load = &ConfigSet::LoadConfig_OneFile<T_ConfigLoader, T>;
	}
	else
	{
		load_helper.func_load = &ConfigSet::LoadConfig_MultiFile<T_ConfigLoader, T>;
	}

	// 保存
	if (config_id < 0)
	{
		_temporary_config_load_helper.push_back(load_helper);
	}
	else
	{
		assert(_config_load_helper.find(config_id) == _config_load_helper.end());
		_config_load_helper[config_id] = load_helper;
	}
}

// 创建动态配置(若已存在，则返回已经存在的，类型匹配失败的话，返回nullptr)
template<typename T>
std::shared_ptr<typename DYNAMIC_CONFIG_MODEL_TYPE(T)> ConfigSet::CreateDynamicConfig()
{
	int32_t conf_id = GET_DYNAMIC_CONFIGID(T);
	auto it = _dynamic_config.find(conf_id);
	if (it != _dynamic_config.end())
	{
		std::shared_ptr<Config<typename DYNAMIC_CONFIG_MODEL_TYPE(T)>> config_ele =
			std::static_pointer_cast<Config<typename DYNAMIC_CONFIG_MODEL_TYPE(T)>>(it->second);
		if (!config_ele)
		{
			return nullptr;
		}

		return config_ele->val;
	}

	// 创建
	std::shared_ptr<Config<typename DYNAMIC_CONFIG_MODEL_TYPE(T)>> o =
		std::make_shared<Config<typename DYNAMIC_CONFIG_MODEL_TYPE(T)>>();
	if (!o)
	{
		assert(false);
		return nullptr;
	}
	_dynamic_config[conf_id] = o;

	return o->val;
}

// 加载
template<typename T_ConfigLoader, typename T>
bool ConfigSet::LoadConfig(const std::string & conf_file_name, std::shared_ptr<Config<typename CONFIG_MODEL_TYPE(T)>> & o)
{
	if (conf_file_name.empty() || !o)
	{
		assert(false);
		return false;
	}

	return T_ConfigLoader::Load(conf_file_name, *((o->val).get()));
}

// 加载（单个文件）
template<typename T_ConfigLoader, typename T>
std::shared_ptr<ConfigSet::ConfigBase> ConfigSet::LoadConfig_OneFile(const std::string & conf_file_name, std::vector<std::string> * err_file_name)
{
	std::shared_ptr<Config<typename CONFIG_MODEL_TYPE(T)>> o = std::make_shared<Config<typename CONFIG_MODEL_TYPE(T)>>();
	std::string file_full_name = _config_dir + conf_file_name;

	if (!LoadConfig<T_ConfigLoader, T>(file_full_name, o))
	{
		if (err_file_name)
		{
			err_file_name->push_back(std::move(file_full_name));
		}
		return nullptr;
	}

	return o;
}

// 加载（多文件）
template<typename T_ConfigLoader, typename T>
std::shared_ptr<ConfigSet::ConfigBase> ConfigSet::LoadConfig_MultiFile(const std::string & conf_file_name, std::vector<std::string> * err_file_name)
{
	std::shared_ptr<Config<typename CONFIG_MODEL_TYPE(T)>> o = std::make_shared<Config<typename CONFIG_MODEL_TYPE(T)>>();
	std::vector<std::string> vec_files = FileHelper::ExpandWildcard(conf_file_name, _config_dir);
	bool ret = true;
	for (std::string & file_name : vec_files)
	{
		if (!LoadConfig<T_ConfigLoader, T>(file_name, o))
		{
			if (err_file_name)
			{
				err_file_name->push_back(std::move(file_name));
			}
			ret = false;
		}
	}

	return (ret ? o : nullptr);
}

// 初始化配置
template<typename T_Obj, typename T_Config>
bool ConfigSet::InitConfig(std::shared_ptr<ConfigSet::ConfigBase> & conf)
{
	std::shared_ptr<Config<T_Config>> config_ele = std::dynamic_pointer_cast<Config<T_Config>>(conf);
	if (!config_ele)
	{
		assert(false);
		return false;
	}

	return ConfigInitializer::Initialize<T_Obj, ConfigSet, T_Config>(*this, *(config_ele->val.get()));
}

}

#endif
