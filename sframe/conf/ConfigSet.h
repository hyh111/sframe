
#ifndef SFRAME_CONFIG_SET_H
#define SFRAME_CONFIG_SET_H

#include <assert.h>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include "ConfigLoader.h"
#include "ConfigMeta.h"

namespace sframe {

// 配置错误信息
struct ConfigError
{
	enum ErrorType
	{
		kLoadConfigError = 1,
		kInitConfigError,
	};

	int32_t err_type;
	int32_t config_type;
	std::string config_file_name;
};

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

	typedef std::shared_ptr<ConfigBase> (ConfigSet::*Func_LoadConfig)(const std::string &);

	typedef bool (ConfigSet::*Func_InitConfig)(std::shared_ptr<ConfigBase> &);

	typedef const char *(*Func_GetConfigName)();

	struct ConfigLoadHelper 
	{
		int32_t conf_id;
		Func_LoadConfig func_load;
		Func_InitConfig func_init;
		std::string conf_file_name;
	};

	ConfigSet() {}

	virtual ~ConfigSet() {}

	// 加载(全部成功返回true, 只要有一个失败都会返回false)
	// 出错时，err_info会返回出错的配置信息
	bool Load(const std::string & path, std::vector<ConfigError> * vec_err_info = nullptr);


	///////////////////////// 查询相关方法 ///////////////////////////

	// 获取配置
	template<typename T>
	std::shared_ptr<const typename CONFIG_MODEL_TYPE(T)> GetConfig()
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
	std::shared_ptr<const T> GetMapConfigItem(const typename CONFIG_KEY_TYPE(T) & key)
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
	std::shared_ptr<const typename DYNAMIC_CONFIG_MODEL_TYPE(T)> GetDynamicConfig()
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
	std::shared_ptr<const T> GetMapDynamicConfigItem(const typename DYNAMIC_CONFIG_KEY_TYPE(T) & key)
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

	/////////////////////////// 注册创建相关

	// 注册配置
	template<typename T_ConfigLoader, typename T>
	void RegistConfig(const std::string & conf_file_name)
	{
		int32_t config_id = GET_CONFIGID(T);
		ConfigLoadHelper load_helper;
		load_helper.conf_id = config_id;
		load_helper.func_load = &ConfigSet::LoadConfig<T_ConfigLoader, T>;
		load_helper.func_init = &ConfigSet::InitConfig<T, typename CONFIG_MODEL_TYPE(T)>;
		load_helper.conf_file_name = conf_file_name;

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
	std::shared_ptr<typename DYNAMIC_CONFIG_MODEL_TYPE(T)> CreateDynamicConfig()
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

private:

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadConfig(const std::string & conf_file_name)
	{
		std::string file_full_name = _config_dir + conf_file_name;
		std::shared_ptr<Config<typename CONFIG_MODEL_TYPE(T)>> o = std::make_shared<Config<typename CONFIG_MODEL_TYPE(T)>>();
		if (!o || !T_ConfigLoader::Load(file_full_name, *((o->val).get())))
		{
			return nullptr;
		}

		return o;
	}

	// 初始化配置
	template<typename T_Obj, typename T_Config>
	bool InitConfig(std::shared_ptr<ConfigBase> & conf)
	{
		std::shared_ptr<Config<T_Config>> config_ele = std::dynamic_pointer_cast<Config<T_Config>>(conf);
		if (!config_ele)
		{
			assert(false);
			return false;
		}

		return ConfigInitializer::Initialize<T_Obj, ConfigSet, T_Config>(*this, *(config_ele->val.get()));
	}

	// 加载并初始化一个配置
	std::shared_ptr<ConfigBase> LoadAndInitConfig(const ConfigLoadHelper & load_helper, std::vector<ConfigError> * err_info);

private:
	std::unordered_map<int32_t, std::shared_ptr<ConfigBase>> _config;
	std::unordered_map<int32_t, ConfigLoadHelper> _config_load_helper;
	std::unordered_map<int32_t, std::shared_ptr<ConfigBase>> _dynamic_config;
	std::vector<ConfigLoadHelper> _temporary_config_load_helper;
	std::string _config_dir;
};

}

#endif
