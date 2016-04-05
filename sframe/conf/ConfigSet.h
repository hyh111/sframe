
#ifndef SFRAME_CONFIG_SET_H
#define SFRAME_CONFIG_SET_H

#include <assert.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "ConfigDef.h"
#include "../util/Lock.h"

namespace sframe {

template<typename T>
bool InitializeConfig(T & conf)
{
	return true;
}

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
			assert(val != nullptr);
		}
		virtual ~Config() {}

		std::shared_ptr<T> val;
	};

	typedef std::shared_ptr<ConfigBase>(ConfigSet::*LoadConfigFunction)();

	typedef bool(*InitConfigFunction)(std::shared_ptr<ConfigBase> &);

	typedef const char *(*GetConfigNameFunction)();

public:

	ConfigSet(int32_t max_count);

	virtual ~ConfigSet();

	// 初始化
	bool Initialize(const std::string & dir, std::string * log_msg = nullptr);

	// 重新加载
	bool Reload(std::string * log_msg = nullptr);

	// 获取配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const T> GetConfig(int32_t config_id)
	{
		if (config_id < 0 || config_id >= _max_count)
		{
			return nullptr;
		}

		AutoLock<SpinLock> l(_lock[config_id]);
		std::shared_ptr<Config<T>> config_ele = std::static_pointer_cast<Config<T>>(_config[config_id]);

		return config_ele ? config_ele->val : nullptr;
	}

	// 获取配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const T> GetConfig()
	{
		return GetConfig<T>(GET_CONFIGID(T));
	}

	// 获取Map类型配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T_Key, typename T_Val>
	std::shared_ptr<const std::unordered_map<T_Key, std::shared_ptr<T_Val>>> GetMapConfig(int32_t config_id)
	{
		return GetConfig<std::unordered_map<T_Key, std::shared_ptr<T_Val>>>(config_id);
	}

	// 获取Map类型配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T_Key, typename T_Val>
	std::shared_ptr<const std::unordered_map<T_Key, std::shared_ptr<T_Val>>> GetMapConfig()
	{
		return GetMapConfig<T_Key, T_Val>(GET_CONFIGID(T_Val));
	}

	// 获取Map类型条目(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T_Key, typename T_Val>
	std::shared_ptr<const T_Val> GetMapConfigItem(int32_t config_id, const T_Key & key)
	{
		std::shared_ptr<const std::unordered_map<T_Key, std::shared_ptr<T_Val>>> map_config = GetMapConfig<T_Key, T_Val>(config_id);
		if (map_config == nullptr)
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

	// 获取Map类型条目(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T_Key, typename T_Val>
	std::shared_ptr<const T_Val> GetMapConfigItem(const T_Key & key)
	{
		return GetMapConfigItem<T_Key, T_Val>(GET_CONFIGID(T_Val), key);
	}

	// 获取Vector类型的配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const std::vector<std::shared_ptr<T>>> GetVectorConfig(int32_t config_id)
	{
		return GetConfig<std::vector<std::shared_ptr<T>>>(config_id);
	}

	// 获取Vector类型的配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const std::vector<std::shared_ptr<T>>> GetVectorConfig()
	{
		return GetVectorConfig<T>(GET_CONFIGID(T));
	}

	// 获取Vector类型的配置条目(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const std::vector<std::shared_ptr<T>>> GetVectorConfigItem(int32_t config_id, int32_t index)
	{
		std::shared_ptr<const std::vector<std::shared_ptr<T>>> v_config = GetVectorConfig<T>(config_id);
		if (v_config == nullptr || index < 0 || index >= v_config->size())
		{
			return nullptr;
		}

		return (*v_config)[index];
	}

	// 获取Vector类型的配置条目(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const std::vector<std::shared_ptr<T>>> GetVectorConfigItem(int32_t index)
	{
		return GetVectorConfigItem(GET_CONFIGID(T), index);
	}

private:

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadObject(int32_t config_id, const std::string & file_name, T_ConfigLoader & loader)
	{
		std::shared_ptr<Config<T>> o;
		if (config_id < 0 || config_id >= _max_count || (o = std::make_shared<Config<T>>()) == nullptr || 
			!loader.Load(_config_dir + file_name, *((o->val).get())))
		{
			return nullptr;
		}

		return o;
	}

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadObject(T_ConfigLoader & loader)
	{
		return LoadObject<T_ConfigLoader, T>(GET_CONFIGID(T), GET_CONFIGFILENAME(T), loader);
	}

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadObject()
	{
		T_ConfigLoader loader;
		return LoadObject<T_ConfigLoader, T>(loader);
	}

	// 加载Map
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	std::shared_ptr<ConfigBase> LoadMap(int32_t config_id, const std::string & file_name, T_ConfigLoader & loader)
	{
		return LoadObject<T_ConfigLoader, std::unordered_map<T_Key, std::shared_ptr<T_Val>>>(config_id, file_name, loader);
	}

	// 加载Map
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	std::shared_ptr<ConfigBase> LoadMap(T_ConfigLoader & loader)
	{
		return LoadMap<T_ConfigLoader, T_Key, T_Val>(GET_CONFIGID(T_Val), GET_CONFIGFILENAME(T_Val), loader);
	}

	// 加载Map
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	std::shared_ptr<ConfigBase> LoadMap()
	{
		T_ConfigLoader loader;
		return LoadMap<T_ConfigLoader, T_Key, T_Val>(loader);
	}

	// 加载Vector
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadVector(int32_t config_id, const std::string & file_name, T_ConfigLoader & loader)
	{
		return LoadObject<T_ConfigLoader, std::vector<std::shared_ptr<T>>>(config_id, file_name, loader);
	}

	// 加载Vector
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadVector(T_ConfigLoader & loader)
	{
		return LoadVector<T_ConfigLoader, T>(GET_CONFIGID(T), GET_CONFIGFILENAME(T), loader);
	}

	// 加载Vector
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<ConfigBase> LoadVector()
	{
		T_ConfigLoader loader;
		return LoadVector<T_ConfigLoader, T>(loader);
	}


protected:

	// 注册所有配置(派生类重写此函数，在其中注册所有的配置)
	virtual void RegistAllConfig() = 0;

	// 初始化配置
	template<typename T>
	static bool InitConfig(std::shared_ptr<ConfigBase> & conf)
	{
		std::shared_ptr<Config<T>> config_ele = std::dynamic_pointer_cast<Config<T>>(conf);
		if (!config_ele)
		{
			assert(false);
			return false;
		}
		return InitializeConfig(*(config_ele->val.get()));
	}

	// 获取配置文件名
	template<typename T>
	static const char * GetConfigName()
	{
		return GET_CONFIGFILENAME(T);
	}

	// 注册对象配置
	template<typename T_ConfigLoader, typename T>
	void RegistObjectConfig()
	{
		int32_t config_id = GET_CONFIGID(T);
		assert(config_id >= 0 && config_id < _max_count);
		assert(!_load_config_function[config_id] && !_init_config_function[config_id] && !_get_config_name_function[config_id]);
		_load_config_function[config_id] = &ConfigSet::LoadObject<T_ConfigLoader, T>;
		_get_config_name_function[config_id] = &ConfigSet::GetConfigName<T>;
		_init_config_function[config_id] = &ConfigSet::InitConfig<T>;
	}

	// 注册MAP配置
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	void RegistMapConfig()
	{
		int32_t config_id = GET_CONFIGID(T_Val);
		assert(config_id >= 0 && config_id < _max_count);
		assert(!_load_config_function[config_id] && !_init_config_function[config_id] && !_get_config_name_function[config_id]);
		_load_config_function[config_id] = &ConfigSet::LoadMap<T_ConfigLoader, T_Key, T_Val>;
		_get_config_name_function[config_id] = &ConfigSet::GetConfigName<T_Val>;
		_init_config_function[config_id] = &ConfigSet::InitConfig<std::unordered_map<T_Key, std::shared_ptr<T_Val>>>;
	}

	// 注册VECTOR配置
	template<typename T_ConfigLoader, typename T>
	void RegistVectorConfig()
	{
		int32_t config_id = GET_CONFIGID(T);
		assert(config_id >= 0 && config_id < _max_count);
		assert(!_load_config_function[config_id] && !_init_config_function[config_id] && !_get_config_name_function[config_id]);
		_load_config_function[config_id] = &ConfigSet::LoadVector<T_ConfigLoader, T>;
		_get_config_name_function[config_id] = &ConfigSet::GetConfigName<T>;
		_init_config_function[config_id] = &ConfigSet::InitConfig<std::vector<std::shared_ptr<T>>>;
	}

private:
	SpinLock * _lock;                           // 每一个配置一个自旋锁
	std::shared_ptr<ConfigBase> * _config;      // 所有配置
	LoadConfigFunction  * _load_config_function;
	GetConfigNameFunction * _get_config_name_function;
	InitConfigFunction * _init_config_function;
	int32_t _max_count;                         // 最大数量
	std::string _config_dir;                    // 配置文件目录
};

}

#endif
