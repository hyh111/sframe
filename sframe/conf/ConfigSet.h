
#ifndef SFRAME_CONFIG_SET_H
#define SFRAME_CONFIG_SET_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "ConfigDef.h"
#include "../util/Lock.h"

namespace sframe {

// 配置集合
class ConfigSet
{
public:

	ConfigSet(int32_t max_count);

	virtual ~ConfigSet();

	// 加载
	bool Load(const std::string & dir);

	// 重新加载
	bool Reload();

	// 获取配置(内部使用强制指针类型转换，务必确保类型正确)
	template<typename T>
	std::shared_ptr<const T> GetConfig(int32_t config_id)
	{
		if (config_id < 0 || config_id >= _max_count)
		{
			return nullptr;
		}

		AutoLock<SpinLock> l(_lock[config_id]);
		return std::static_pointer_cast<const T>(_config[config_id]);
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

protected:

	// 加载(派生类实现此函数，以实现所有配置的加载)
	virtual bool LoadAll() = 0;

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<T> LoadConfig(int32_t config_id, const std::string & file_name, T_ConfigLoader & loader)
	{
		std::shared_ptr<T> o;
		if (config_id < 0 || config_id >= _max_count || _temp[config_id] != nullptr ||
			(o = std::make_shared<T>()) == nullptr || !loader.Load(_config_dir + file_name, *(o.get())))
		{
			return nullptr;
		}

		_temp[config_id] = o;

		return o;
	}

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<T> LoadConfig(T_ConfigLoader & loader)
	{
		return LoadConfig<T_ConfigLoader, T>(GET_CONFIGID(T), GET_CONFIGFILENAME(T), loader);
	}

	// 加载
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<T> LoadConfig()
	{
		T_ConfigLoader loader;
		return LoadConfig<T_ConfigLoader, T>(loader);
	}

	// 加载Map
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	std::shared_ptr<std::unordered_map<T_Key, std::shared_ptr<T_Val>>> LoadMap(int32_t config_id, const std::string & file_name, T_ConfigLoader & loader)
	{
		return LoadConfig<T_ConfigLoader, std::unordered_map<T_Key, std::shared_ptr<T_Val>>>(config_id, file_name, loader);
	}

	// 加载Map
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	std::shared_ptr<std::unordered_map<T_Key, std::shared_ptr<T_Val>>> LoadMap(T_ConfigLoader & loader)
	{
		return LoadMap<T_ConfigLoader, T_Key, T_Val>(GET_CONFIGID(T_Val), GET_CONFIGFILENAME(T_Val), loader);
	}

	// 加载Map
	template<typename T_ConfigLoader, typename T_Key, typename T_Val>
	std::shared_ptr<std::unordered_map<T_Key, std::shared_ptr<T_Val>>> LoadMap()
	{
		T_ConfigLoader loader;
		return LoadMap<T_ConfigLoader, T_Key, T_Val>(loader);
	}

	// 加载Vector
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<std::vector<std::shared_ptr<T>>> LoadVector(int32_t config_id, const std::string & file_name, T_ConfigLoader & loader)
	{
		return LoadConfig<T_ConfigLoader, std::vector<std::shared_ptr<T>>>(config_id, file_name, loader);
	}

	// 加载Vector
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<std::vector<std::shared_ptr<T>>> LoadVector(T_ConfigLoader & loader)
	{
		return LoadVector<T_ConfigLoader, T>(GET_CONFIGID(T), GET_CONFIGFILENAME(T), loader);
	}

	// 加载Vector
	template<typename T_ConfigLoader, typename T>
	std::shared_ptr<std::vector<std::shared_ptr<T>>> LoadVector()
	{
		T_ConfigLoader loader;
		return LoadVector<T_ConfigLoader, T>(loader);
	}

private:
	// 应用配置（将临时的配置替换到正式配置）
	void Apply();

private:
	SpinLock * _lock;                     // 每一个配置一个自旋锁
	std::shared_ptr<void> * _config;      // 所有配置
	std::shared_ptr<void> * _temp;        // 临时的
	int32_t _max_count;                   // 最大数量
	std::string _config_dir;              // 配置文件目录
};

}

#endif
