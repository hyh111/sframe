
#ifndef SFRAME_CONFIG_META_H
#define SFRAME_CONFIG_META_H

#include <inttypes.h>
#include <vector>
#include <set>
#include <map>

namespace sframe {

class ConfigModule
{
public:

	ConfigModule() {}

	virtual ~ConfigModule() {}

};

// 配置模块基类
template<typename T_ConfigModel, int Conf_Id>
class ConfigModuleT : public ConfigModule
{
public:

	typedef T_ConfigModel ModelType;

	static int GetConfigId()
	{
		return Conf_Id;
	}

	ConfigModuleT()
	{
		_conf_obj = std::make_shared<ModelType>();
	}

	const std::shared_ptr<ModelType> & Obj() const
	{
		return _conf_obj;
	}

private:
	std::shared_ptr<ModelType> _conf_obj;
};

// 单一对象配置模块
template<typename T_Config, int Conf_Id>
class ObjectConfigModule : public ConfigModuleT<T_Config, Conf_Id>
{
public:
	typedef T_Config ConfType;
};

// vector配置模块
template<typename T_Config, int Conf_Id>
class VectorConfigModule : public ConfigModuleT<std::vector<std::shared_ptr<T_Config>>, Conf_Id>
{
public:
	typedef T_Config ConfType;
};

// set配置模块
template<typename T_Config, int Conf_Id>
class SetConfigModule : public ConfigModuleT<std::set<std::shared_ptr<T_Config>>, Conf_Id>
{
public:
	typedef T_Config ConfType;
};

// map配置模块
template<typename T_Key, typename T_Config, int Conf_Id>
class MapConfigModule : public ConfigModuleT<std::map<T_Key, std::shared_ptr<T_Config>>, Conf_Id>
{
public:

	typedef T_Key KeyType;

	typedef T_Config ConfType;

	std::shared_ptr<ConfType> GetConfigItemNotConst(const KeyType & k) const
	{
		auto it = this->Obj()->find(k);
		return (it == this->Obj()->end() ? nullptr : it->second);
	}

	std::shared_ptr<const T_Config> GetConfigItem(const KeyType & k) const
	{
		auto it = this->Obj()->find(k);
		return (it == this->Obj()->end() ? nullptr : it->second);
	}

};


class GetConfigObjKey_Warpper
{
	template<typename R, typename T, R(T::*)() const>
	struct MethodMatcher;

	template<typename R, typename T>
	static std::true_type match(MethodMatcher<R, T, &T::GetKey>*) { return std::true_type(); }

	template<typename R, typename T>
	static std::false_type match(...) { return std::false_type(); }

	template<typename R, typename T>
	inline static R call(std::false_type, T & obj)
	{
		return R();
	}

	template<typename R, typename T>
	inline static R call(std::true_type, T & obj)
	{
		return obj.GetKey();
	}

public:
	template<typename R, typename T>
	inline static R GetKey(T & obj)
	{
		return call<R, T>(decltype(match<R, T>(nullptr))(), obj);
	}
};

template<typename R, typename T>
inline R GetConfigObjKey(T & obj)
{
	return GetConfigObjKey_Warpper::GetKey<R>(obj);
}

template<typename R, typename T>
inline R GetConfigObjKey(std::shared_ptr<T> & obj)
{
	return GetConfigObjKey_Warpper::GetKey<R>(*obj);
}

}


// 声明单一对象模型的配置
// OBJ_CONFIG_MODULE(配置模块名, 配置结构体名, 配置ID)
#define OBJ_CONFIG_MODULE(module, conf, conf_id) class module : public sframe::ObjectConfigModule<conf, conf_id> {};

// 声明Vetor模型的配置
// VECTOR_CONFIG_MODULE(配置模块名, 结构体名, 配置ID)
#define VECTOR_CONFIG_MODULE(module, conf, conf_id) class module : public sframe::VectorConfigModule<conf, conf_id> {};

// 声明Set模型的配置
// SET_CONFIG_MODULE(配置模块名, 结构体名, 配置ID)
#define SET_CONFIG_MODULE(module, conf, conf_id) class module : public sframe::SetConfigModule<conf, conf_id> {};

// 声明Map模型的配置
// MAP_CONFIG_MODULE(配置模块名, key类型, 结构体名, 结构体中用作key的成员变量名, 配置ID)
#define MAP_CONFIG_MODULE(module, k, conf, conf_id) class module : public sframe::MapConfigModule<k, conf, (conf_id)> {};

// 为MAP类型配置类指定最为key的字段
#define KEY_FIELD(k_type, k_field) k_type GetKey() const {return k_field;}

// 获取配置key类型
#define CONFIG_KEY_TYPE(module) module::KeyType

// 获取配置模型类型
#define CONFIG_MODEL_TYPE(module) module::ModelType

// 获取配置类型
#define CONFIG_CONF_TYPE(module) module::ConfType

// 获取配置ID
#define GET_CONFIGID(module) module::GetConfigId()

#endif