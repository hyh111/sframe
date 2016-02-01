
#ifndef SFRAME_CONFIG_STRING_PARSER_H
#define SFRAME_CONFIG_STRING_PARSER_H

#include <vector>
#include <list>
#include <unordered_map>
#include <map>
#include <memory>
#include "../util/Convert.h"
#include "../util/StringHelper.h"

template<typename T>
void ParseConfigString(const std::string & str, T & obj)
{
	obj = sframe::StrToAny<T>(str);
}

template<typename T_Map>
void ParseMap(const std::string & str, T_Map & obj)
{
	static const char kMapItemSep = ';';
	static const char kMapKVSep = '_';

	if (str.empty())
	{
		return;
	}

	// 分割每个条目
	std::string sep_str(sframe::GetCharMaxContinueInString(str, kMapItemSep), kMapItemSep);
	std::vector<std::string> all_items;
	sframe::SplitString(str, all_items, sep_str);

	for (auto const & item : all_items)
	{
		if (item.empty())
		{
			continue;
		}

		std::string kv_sep(sframe::GetCharMaxContinueInString(item, kMapKVSep), kMapKVSep);
		if (kv_sep.empty())
		{
			continue;
		}

		size_t pos = item.find(kv_sep, 0);
		if (pos == std::string::npos)
		{
			pos = item.length();
		}

		assert(pos <= item.length());

		std::string keystr = std::move(item.substr(0, pos));
		std::string valstr;
		if (pos < item.length())
		{
			valstr = std::move(item.substr(pos + 1, item.length() - pos - 1));
		}
		
		if (keystr.empty())
		{
			continue;
		}

		typename T_Map::key_type k;
		typename T_Map::mapped_type v;

		ParseConfigString(keystr, k);
		ParseConfigString(valstr, v);
		obj.insert(std::make_pair(k, v));
	}
}

template<typename T_Array>
void ParseArray(const std::string & str, T_Array & obj)
{
	static const char kSep = ',';

	if (str.empty())
	{
		return;
	}

	// 分割每个条目
	std::string sep_str(sframe::GetCharMaxContinueInString(str, kSep), kSep);
	std::vector<std::string> all_items;
	sframe::SplitString(str, all_items, sep_str);

	for (auto const & item : all_items)
	{
		if (item.empty())
		{
			continue;
		}

		typename T_Array::value_type v;

		ParseConfigString(item, v);
		obj.push_back(v);
	}
}

template<typename T, int Array_Size>
void ParseConfigString(const std::string & str, T(&obj)[Array_Size])
{
	static const char kSep = ',';

	if (str.empty())
	{
		return;
	}

	// 分割每个条目
	std::string sep_str(sframe::GetCharMaxContinueInString(str, kSep), kSep);
	std::vector<std::string> all_items;
	sframe::SplitString(str, all_items, sep_str);

	int len = (int)all_items.size() > Array_Size ? Array_Size : (int)all_items.size();

	for (int i = 0; i < len; i++)
	{
		if (all_items[i].empty())
		{
			continue;
		}

		ParseConfigString(all_items[i], obj[i]);
	}
}

template<typename T_Key, typename T_Val>
void ParseConfigString(const std::string & str, std::unordered_map<T_Key, T_Val> & obj)
{
	ParseMap(str, obj);
}

template<typename T_Key, typename T_Val>
void ParseConfigString(const std::string & str, std::map<T_Key, T_Val> & obj)
{
	ParseMap(str, obj);
}

template<typename T>
void ParseConfigString(const std::string & str, std::vector<T> & obj)
{
	ParseArray(str, obj);
}

template<typename T>
void ParseConfigString(const std::string & str, std::list<T> & obj)
{
	ParseArray(str, obj);
}

template<typename T>
void ParseConfigString(const std::string & str, std::shared_ptr<T> & obj)
{
	obj = std::make_shared<T>();
	Parse(str, *(obj.get()));
}

#endif