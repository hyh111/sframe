
#ifndef SFRAME_JSON_READER_H
#define SFRAME_JSON_READER_H

#include <assert.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>
#include "../util/json11.hpp"
#include "../util/Convert.h"

// 读取JSON到任意类型基础类型
template<typename T>
inline void Json_FillObject(const json11::Json & json, T & obj)
{
	switch (json.type())
	{
		case json11::Json::Type::BOOL:
		{
			obj = static_cast<T>(json.bool_value());
		}
		break;

		case json11::Json::Type::NUMBER:
		{
			obj = static_cast<T>(json.number_value());
		}
		break;

		case json11::Json::Type::STRING:
		{
			obj = sframe::StrToAny<T>(json.string_value());
		}
		break;
	}
}

template<>
inline void Json_FillObject(const json11::Json &json, std::string & obj)
{
	switch (json.type())
	{
		case json11::Json::Type::BOOL:
		{
			obj = std::to_string(json.bool_value());
		}
		break;

		case json11::Json::Type::NUMBER:
		{
			obj = std::to_string(json.number_value());
		}
		break;

		case json11::Json::Type::STRING:
		{
			obj = json.string_value();
		}
		break;
	}
}

template<typename R, typename T>
inline R Json_GetObjKey(T & obj)
{
	return obj.GetKey();
}

template<typename R, typename T>
inline R Json_GetObjKey(std::shared_ptr<T> & obj)
{
	return obj->GetKey();
}

template<typename T>
inline void Json_FillMap(const json11::Json & json, T & obj)
{
	switch (json.type())
	{
		case json11::Json::OBJECT:
		{
			auto & m = json.object_items();
			for (auto & item : m)
			{
				typename T::key_type k;
				typename T::mapped_type v;

				k = sframe::StrToAny<T::key_type>(item.first);
				Json_FillObject(item.second, v);
				obj.insert(std::make_pair(k, v));
			}
		}
		break;

		case json11::Json::ARRAY:
		{
			auto & arr = json.array_items();
			for (auto & item : arr)
			{
				typename T::mapped_type v;
				Json_FillObject(item, v);

				typename T::key_type k = Json_GetObjKey<typename T::key_type>(v);
				obj.insert(std::make_pair(k, v));
			}
		}
		break;
	}
}

template<typename T>
inline void Json_FillArray(const json11::Json & json, T & obj)
{
	if (!json.is_array())
	{
		return;
	}

	auto & arr = json.array_items();
	for (auto & item : arr)
	{
		typename T::value_type v;
		Json_FillObject(item, v);
		obj.push_back(v);
	}
}

// 读取JSON到数组
template<typename T, int Array_Size>
inline void Json_FillObject(const json11::Json & json, T(&obj)[Array_Size])
{
	if (!json.is_array())
	{
		return;
	}

	auto & arr = json.array_items();
	if ((int)arr.size() != Array_Size)
	{
		return;
	}

	for (int i = 0; i < Array_Size; i++)
	{
		Json_FillObject(arr[i], obj[i]);
	}
}

// 读取JSON到unorder_map
template<typename T_Key, typename T_Val>
inline void Json_FillObject(const json11::Json & json, std::unordered_map<T_Key, T_Val> & obj)
{
	Json_FillMap(json, obj);
}

// 读取JSON到map
template<typename T_Key, typename T_Val>
inline void Json_FillObject(const json11::Json & json, std::map<T_Key, T_Val> & obj)
{
	Json_FillMap(json, obj);
}

// 读取JSON到vector
template<typename T>
inline void Json_FillObject(const json11::Json & json, std::vector<T> & obj)
{
	Json_FillArray(json, obj);
}

// 读取JSON到list
template<typename T>
inline void Json_FillObject(const json11::Json & json, std::list<T> & obj)
{
	Json_FillArray(json, obj);
}

// 读取JSON到shared_ptr
template<typename T>
inline void Json_FillObject(const json11::Json & json, std::shared_ptr<T> & obj)
{
	obj = std::make_shared<T>();
	Json_FillObject(json, *(obj.get()));
}

// 读取JSON字段
template<typename T>
inline void Json_FillFieldWithDefault(const json11::Json & json, const char * field_name, T & obj, const T & default_val = T())
{
	if (!json.is_object() || json[field_name].is_null())
	{
		obj = default_val;
		return;
	}

	Json_FillObject(json[field_name], obj);
}

// 读取JSON字段
template<typename T>
inline void Json_FillFieldWithDefault(const json11::Json & json, int field_index, T & obj, const T & default_val = T())
{
	if (!json.is_array() || json[field_index].is_null())
	{
		obj = default_val;
		return;
	}

	Json_FillObject(json[field_index], obj);
}

// 读取JSON字段
template<typename T>
inline void Json_FillField(const json11::Json & json, const char * field_name, T & obj)
{
	if (!json.is_object())
	{
		return;
	}

	return Json_FillObject(json[field_name], obj);
}

// 读取JSON字段
template<typename T>
inline void Json_FillField(const json11::Json & json, int field_index, T & obj)
{
	if (!json.is_array())
	{
		return;
	}

	return Json_FillObject(json[field_index], obj);
}

#define JSON_FILLFIELD(name)                              Json_FillField(json, #name, obj.name);
#define JSON_FILLFIELD_WITH_DEFAULT(name, defaultval)     Json_FillFieldWithDefault(json, #name, obj.name, defaultval)

#define JSON_FILLFIELD_INDEX(index, name)                       Json_FillField(json, (int)index, obj.name);
#define JSON_FILLFIELD_INDEX_WITH_DEFAULT(index, name, defaultval)     Json_FillFieldWithDefault(json, (int)index, obj.name, defaultval)

#endif
