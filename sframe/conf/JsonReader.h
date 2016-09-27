
#ifndef SFRAME_JSON_READER_H
#define SFRAME_JSON_READER_H

#include <assert.h>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <memory>
#include "ConfigLoader.h"
#include "ConfigMeta.h"
#include "../util/json11.hpp"
#include "../util/Convert.h"

namespace sframe {

// 读取JSON到任意类型基础类型
template<typename T>
struct ObjectFiller<const json11::Json, T>
{
	static bool Fill(const json11::Json & json, T & obj)
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

		default:
		{
			return false;
		}
		}

		return true;
	}
};

template<>
struct ObjectFiller<const json11::Json, std::string>
{
	static bool Fill(const json11::Json &json, std::string & obj)
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

		default:
		{
			return false;
		}
		}

		return true;
	}
};

// 读取JSON到数组
template<typename T, int Array_Size>
struct ObjectFiller<const json11::Json, T[Array_Size]>
{
	static bool Fill(const json11::Json & json, T(&obj)[Array_Size])
	{
		if (!json.is_array())
		{
			return false;
		}

		auto & arr = json.array_items();
		if ((int)arr.size() != Array_Size)
		{
			return false;
		}

		for (int i = 0; i < Array_Size; i++)
		{
			if (!ConfigLoader::Load(arr[i], obj[i]))
			{
				return false;
			}
		}

		return true;
	}
};

template<typename T>
inline bool Json_FillMap(const json11::Json & json, T & obj)
{
	switch (json.type())
	{
	case json11::Json::OBJECT:
	{
		auto & m = json.object_items();
		for (auto & item : m)
		{
			typename T::mapped_type v;
			if (!ConfigLoader::Load(item.second, v))
			{
				return false;
			}

			typename T::key_type k = sframe::StrToAny<typename T::key_type>(item.first);
			if (!obj.insert(std::make_pair(k, v)).second)
			{
				return false;
			}
		}
	}
	break;

	case json11::Json::ARRAY:
	{
		auto & arr = json.array_items();
		for (auto & item : arr)
		{
			typename T::mapped_type v;
			if (!ConfigLoader::Load(item, v))
			{
				return false;
			}

			typename T::key_type k = GetConfigObjKey<typename T::key_type>(v);
			if (!obj.insert(std::make_pair(k, v)).second)
			{
				return false;
			}
		}
	}
	break;

	default:
	{
		return false;
	}
	}

	return true;
}

// 读取JSON到unorder_map
template<typename T_Key, typename T_Val>
struct ObjectFiller<const json11::Json, std::unordered_map<T_Key, T_Val>>
{
	static bool Fill(const json11::Json & json, std::unordered_map<T_Key, T_Val> & obj)
	{
		return Json_FillMap(json, obj);
	}
};

// 读取JSON到map
template<typename T_Key, typename T_Val>
struct ObjectFiller<const json11::Json, std::map<T_Key, T_Val>>
{
	static bool Fill(const json11::Json & json, std::map<T_Key, T_Val> & obj)
	{
		return Json_FillMap(json, obj);
	}
};

template<typename T>
inline bool Json_FillArray(const json11::Json & json, T & obj)
{
	if (json.is_array())
	{
		auto & arr = json.array_items();
		for (auto & item : arr)
		{
			typename T::value_type v;
			if (!ConfigLoader::Load(item, v))
			{
				return false;
			}

			obj.push_back(v);
		}
	}
	else
	{
		typename T::value_type v;
		if (!ConfigLoader::Load(json, v))
		{
			return false;
		}

		obj.push_back(v);
	}

	return true;
}

// 读取JSON到vector
template<typename T>
struct ObjectFiller<const json11::Json, std::vector<T>>
{
	static bool Fill(const json11::Json & json, std::vector<T> & obj)
	{
		return Json_FillArray(json, obj);
	}
};

// 读取JSON到list
template<typename T>
struct ObjectFiller<const json11::Json, std::list<T>>
{
	static bool Fill(const json11::Json & json, std::list<T> & obj)
	{
		return Json_FillArray(json, obj);
	}
};

template<typename T_Set>
inline bool Json_FillSet(const json11::Json & json, T_Set & obj)
{
	if (json.is_array())
	{
		auto & arr = json.array_items();
		for (auto & item : arr)
		{
			typename T_Set::value_type v;
			if (!ConfigLoader::Load(item, v))
			{
				return false;
			}

			obj.insert(v);
		}
	}
	else
	{
		typename T_Set::value_type v;
		if (!ConfigLoader::Load(json, v))
		{
			return false;
		}

		obj.insert(v);
	}

	return true;
}

// 填充Table到set
template<typename T>
struct ObjectFiller<const json11::Json, std::set<T>>
{
	static bool Fill(const json11::Json & json, std::set<T> & obj)
	{
		return Json_FillSet(json, obj);
	}
};

// 填充Table到unordered_set
template<typename T>
struct ObjectFiller<const json11::Json, std::unordered_set<T>>
{
	static bool Fill(const json11::Json & json, std::unordered_set<T> & obj)
	{
		return Json_FillSet(json, obj);
	}
};

// 读取JSON到shared_ptr
template<typename T>
struct ObjectFiller<const json11::Json, std::shared_ptr<T>>
{
	static bool Fill(const json11::Json & json, std::shared_ptr<T> & obj)
	{
		obj = std::make_shared<T>();
		return ConfigLoader::Load(json, *(obj.get()));
	}
};


// 读取JSON字段
template<typename T>
inline bool FillField(const json11::Json & json, const char * field_name, T & obj, const T & default_val = T())
{
	if (!json.is_object() || json[field_name].is_null())
	{
		obj = default_val;
		return false;
	}

	if (!ConfigLoader::Load(json[field_name], obj))
	{
		obj = default_val;
		return false;
	}

	return true;
}

// 读取JSON字段
template<typename T>
inline bool FillIndex(const json11::Json & json, int field_index, T & obj, const T & default_val = T())
{
	if (!json.is_array() || json[field_index].is_null())
	{
		obj = default_val;
		return false;
	}

	if (!ConfigLoader::Load(json[field_index], obj))
	{
		obj = default_val;
		return false;
	}

	return true;
}

}

#endif
