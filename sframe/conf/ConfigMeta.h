
#ifndef SFRAME_CONFIG_META_H
#define SFRAME_CONFIG_META_H

#include <inttypes.h>
#include <vector>
#include <set>
#include <map>

namespace sframe {


#define GET_CONFIGID(T) T::ConfigMeta::GetConfigId()

#define CONFIG_MODEL_TYPE(T) T::ConfigMeta::ModelType

#define CONFIG_KEY_TYPE(T) T::ConfigMeta::KeyType

#define GET_DYNAMIC_CONFIGID(T) T::DynamicConfigMeta::GetConfigId()

#define DYNAMIC_CONFIG_MODEL_TYPE(T) T::DynamicConfigMeta::ModelType

#define DYNAMIC_CONFIG_KEY_TYPE(T) T::DynamicConfigMeta::KeyType

class GetConfigObjKey_Warpper
{
	template<typename R, typename T, R(*)(const T&)>
	struct MethodMatcher;

	template<typename R, typename T>
	static std::true_type match(MethodMatcher<R, T, &T::ConfigMeta::GetKey>*) {}

	template<typename R, typename T>
	static std::false_type match(...) {}

	template<typename R, typename T>
	inline static R call(std::false_type, T & obj)
	{
		return R();
	}

	template<typename R, typename T>
	inline static R call(std::true_type, T & obj)
	{
		return T::ConfigMeta::GetKey(obj);
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

#endif