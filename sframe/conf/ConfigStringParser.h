
#ifndef SFRAME_CONFIG_STRING_PARSER_H
#define SFRAME_CONFIG_STRING_PARSER_H

#include <vector>
#include <list>
#include <unordered_map>
#include <map>
#include <memory>
#include "../util/Convert.h"
#include "../util/StringHelper.h"

namespace sframe {

template<typename T>
struct TblStrParser
{
	static void ParseTableString(const std::string & str, T & obj)
	{
		obj = sframe::StrToAny<T>(str);
	}
};

class ParseCaller
{
public:
	template<typename T>
	static void Parse(const std::string & str, T & obj)
	{
		return call<T>(decltype(match<T>(nullptr))(), str, obj);
	}

private:
	template<typename T>
	static void call(std::false_type, const std::string & str, T & obj)
	{
		TblStrParser<T>::ParseTableString(str, obj);
	}

	template<typename T>
	static void call(std::true_type, const std::string & str, T & obj)
	{
		obj.ParseTableString(str);
	}

	// 匹配器 ———— bool返回值类成员函数，形如 bool T_Obj::FillObject(T_Reader & reader)
	template<typename U, void(U::*)(const std::string &) const>
	struct MethodMatcher;

	template<typename U>
	static std::true_type match(MethodMatcher<U, &U::ParseTableString>*);

	template<typename U>
	static std::false_type match(...);
};


template<typename T_Map>
void ParseMap(const std::string & str, T_Map & obj)
{
	static const char kMapItemSep = ';';
	static const char kMapKVSep = '#';

	if (str.empty())
	{
		return;
	}

	// 分割每个条目
	std::string sep_str(sframe::GetCharMaxContinueInString(str, kMapItemSep), kMapItemSep);
	std::vector<std::string> all_items = sframe::SplitString(str, sep_str);

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

		ParseCaller::Parse(keystr, k);
		ParseCaller::Parse(valstr, v);
		obj.insert(std::make_pair(k, v));
	}
}

template<typename T_Array>
void ParseArray(const std::string & str, T_Array & obj)
{
	static const char kSep = '|';

	if (str.empty())
	{
		return;
	}

	// 分割每个条目
	std::string sep_str(sframe::GetCharMaxContinueInString(str, kSep), kSep);
	std::vector<std::string> all_items = sframe::SplitString(str, sep_str);

	for (auto const & item : all_items)
	{
		if (item.empty())
		{
			continue;
		}

		typename T_Array::value_type v;

		ParseCaller::Parse(item, v);
		obj.push_back(v);
	}
}

template<typename T, int Array_Size>
struct TblStrParser<T[Array_Size]>
{
	static void ParseTableString(const std::string & str, T(&obj)[Array_Size])
	{
		static const char kSep = ',';

		if (str.empty())
		{
			return;
		}

		// 分割每个条目
		std::string sep_str(sframe::GetCharMaxContinueInString(str, kSep), kSep);
		std::vector<std::string> all_items = sframe::SplitString(str, sep_str);

		int len = (int)all_items.size() > Array_Size ? Array_Size : (int)all_items.size();

		for (int i = 0; i < len; i++)
		{
			if (all_items[i].empty())
			{
				continue;
			}

			ParseCaller::Parse(all_items[i], obj[i]);
		}
	}
};

template<typename T_Key, typename T_Val>
struct TblStrParser<std::unordered_map<T_Key, T_Val>>
{
	static void ParseTableString(const std::string & str, std::unordered_map<T_Key, T_Val> & obj)
	{
		ParseMap(str, obj);
	}
};

template<typename T_Key, typename T_Val>
struct TblStrParser<std::map<T_Key, T_Val>>
{
	static void ParseTableString(const std::string & str, std::map<T_Key, T_Val> & obj)
	{
		ParseMap(str, obj);
	}
};

template<typename T>
struct TblStrParser<std::vector<T>>
{
	static void ParseTableString(const std::string & str, std::vector<T> & obj)
	{
		ParseArray(str, obj);
	}
};

template<typename T>
struct TblStrParser<std::list<T>>
{
	static void ParseTableString(const std::string & str, std::list<T> & obj)
	{
		ParseArray(str, obj);
	}
};

template<typename T>
struct TblStrParser<std::shared_ptr<T>>
{
	static void ParseTableString(const std::string & str, std::shared_ptr<T> & obj)
	{
		obj = std::make_shared<T>();
		ParseCaller::Parse(str, *(obj.get()));
	}
};

}

#endif