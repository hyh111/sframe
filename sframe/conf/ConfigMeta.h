
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

// ÅäÖÃÄ£¿é»ùÀà
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

// µ¥Ò»¶ÔÏóÅäÖÃÄ£¿é
template<typename T_Config, int Conf_Id>
class ObjectConfigModule : public ConfigModuleT<T_Config, Conf_Id>
{
public:
	typedef T_Config ConfType;
};

// vectorÅäÖÃÄ£¿é
template<typename T_Config, int Conf_Id>
class VectorConfigModule : public ConfigModuleT<std::vector<std::shared_ptr<T_Config>>, Conf_Id>
{
public:
	typedef T_Config ConfType;
};

// setÅäÖÃÄ£¿é
template<typename T_Config, int Conf_Id>
class SetConfigModule : public ConfigModuleT<std::set<std::shared_ptr<T_Config>>, Conf_Id>
{
public:
	typedef T_Config ConfType;
};

// mapÅäÖÃÄ£¿é
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


// ÉùÃ÷µ¥Ò»¶ÔÏóÄ£ÐÍµÄÅäÖÃ
// OBJ_CONFIG_MODULE(ÅäÖÃÄ£¿éÃû, ÅäÖÃ½á¹¹ÌåÃû, ÅäÖÃID)
#define OBJ_CONFIG_MODULE(module, conf, conf_id) class module : public sframe::ObjectConfigModule<conf, conf_id> {};

// ÉùÃ÷VetorÄ£ÐÍµÄÅäÖÃ
// VECTOR_CONFIG_MODULE(ÅäÖÃÄ£¿éÃû, ½á¹¹ÌåÃû, ÅäÖÃID)
#define VECTOR_CONFIG_MODULE(module, conf, conf_id) class module : public sframe::VectorConfigModule<conf, conf_id> {};

// ÉùÃ÷SetÄ£ÐÍµÄÅäÖÃ
// SET_CONFIG_MODULE(ÅäÖÃÄ£¿éÃû, ½á¹¹ÌåÃû, ÅäÖÃID)
#define SET_CONFIG_MODULE(module, conf, conf_id) class module : public sframe::SetConfigModule<conf, conf_id> {};

// ÉùÃ÷MapÄ£ÐÍµÄÅäÖÃ
// MAP_CONFIG_MODULE(ÅäÖÃÄ£¿éÃû, keyÀàÐÍ, ½á¹¹ÌåÃû, ½á¹¹ÌåÖÐÓÃ×÷keyµÄ³ÉÔ±±äÁ¿Ãû, ÅäÖÃID)
#define MAP_CONFIG_MODULE(module, k, conf, conf_id) class module : public sframe::MapConfigModule<k, conf, (conf_id)> {};

// ÎªMAPÀàÐÍÅäÖÃÀàÖ¸¶¨×îÎªkeyµÄ×Ö¶Î
#define KEY_FIELD(k_type, k_field) k_type GetKey() const {return k_field;}

// »ñÈ¡ÅäÖÃkeyÀàÐÍ
#define CONFIG_KEY_TYPE(module) module::KeyType

// »ñÈ¡ÅäÖÃÄ£ÐÍÀàÐÍ
#define CONFIG_MODEL_TYPE(module) module::ModelType

// »ñÈ¡ÅäÖÃÀàÐÍ
#define CONFIG_CONF_TYPE(module) module::ConfType

// »ñÈ¡ÅäÖÃID
#define GET_CONFIGID(module) module::GetConfigId()

#endif