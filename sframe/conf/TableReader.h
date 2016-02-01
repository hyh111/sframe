
#ifndef SFRAME_TABLE_READER_H
#define SFRAME_TABLE_READER_H

#include "Table.h"
#include "ConfigStringParser.h"

class TableReader
{
public:
	TableReader(sframe::Table & tbl) : _tbl(&tbl), _cur_row(0) {}
	~TableReader() {}

	sframe::Row * GetCurrentRow()
	{
		if (_cur_row >= _tbl->GetRowCount())
		{
			return nullptr;
		}

		return &(*_tbl)[_cur_row];
	}

	void Next()
	{
		if (_cur_row >= _tbl->GetRowCount())
		{
			return;
		}
		_cur_row++;
	}

private:
	sframe::Table * _tbl;
	int32_t _cur_row;
};

template<typename R, typename T>
inline R Table_GetObjKey(T & obj)
{
	return obj.GetKey();
}

template<typename R, typename T>
inline R Table_GetObjKey(std::shared_ptr<T> & obj)
{
	return obj->GetKey();
}

template<typename T_Map>
inline void Table_FillMap(TableReader & tbl, T_Map & obj)
{
	while (true)
	{
		if (!tbl.GetCurrentRow())
		{
			break;
		}

		typename T_Map::mapped_type v;
		Table_FillObject(tbl, v);

		typename T_Map::key_type k = Table_GetObjKey<typename T_Map::key_type>(v);
		obj.insert(std::make_pair(k, v));

		tbl.Next();
	}
}

template<typename T_Array>
inline void Table_FillArray(TableReader & tbl, T_Array & obj)
{
	while (true)
	{
		if (!tbl.GetCurrentRow())
		{
			break;
		}

		typename T_Array::value_type v;
		Table_FillObject(tbl, v);

		obj.push_back(v);
		tbl.Next();
	}
}

// Ìî³äTableµ½unorder_map
template<typename T_Key, typename T_Val>
inline void Table_FillObject(TableReader & tbl, std::unordered_map<T_Key, T_Val> & obj)
{
	Table_FillMap(tbl, obj);
}

// Ìî³äTableµ½map
template<typename T_Key, typename T_Val>
inline void Table_FillObject(TableReader & tbl, std::map<T_Key, T_Val> & obj)
{
	Table_FillMap(tbl, obj);
}

// Ìî³äTableµ½vector
template<typename T>
inline void Table_FillObject(TableReader & tbl, std::vector<T> & obj)
{
	Table_FillArray(tbl, obj);
}

// Ìî³äTableµ½list
template<typename T>
inline void Table_FillObject(TableReader & tbl, std::list<T> & obj)
{
	Table_FillArray(tbl, obj);
}

// Ìî³äTableµ½shared_ptr
template<typename T>
inline void Table_FillObject(TableReader & tbl, std::shared_ptr<T> & obj)
{
	obj = std::make_shared<T>();
	Table_FillObject(tbl, *(obj.get()));
}

// Ìî³ä±í¸ñ×Ö¶Î
template<typename T>
inline void Table_FillFieldWithDefault(TableReader & tbl, const char * field_name, T & obj, const T & default_value)
{
	std::string * str = nullptr;
	sframe::Row * r = tbl.GetCurrentRow();
	if (!r || (str = r->GetValue(field_name)) == nullptr)
	{
		obj = default_value;
		return;
	}
	
	ParseConfigString(*str, obj);
}

// Ìî³ä±í¸ñ×Ö¶Î
template<typename T>
inline void Table_FillFieldWithDefault(TableReader & tbl, int32_t field_index, T & obj, const T & default_value)
{
	std::string * str = nullptr;
	sframe::Row * r = tbl.GetCurrentRow();
	if (!r || (str = r->GetValue(field_index, false)) == nullptr)
	{
		obj = default_value;
		return;
	}

	ParseConfigString(*str, obj);
}

// Ìî³ä±í¸ñ×Ö¶Î
template<typename T>
inline void Table_FillField(TableReader & tbl, const char * field_name, T & obj)
{
	std::string * str = nullptr;
	sframe::Row * r = tbl.GetCurrentRow();
	if (!r || (str = r->GetValue(field_name)) == nullptr)
	{
		return;
	}

	ParseConfigString(*str, obj);
}

// Ìî³ä±í¸ñ×Ö¶Î
template<typename T>
inline void Table_FillField(TableReader & tbl, int32_t field_index, T & obj)
{
	std::string * str = nullptr;
	sframe::Row * r = tbl.GetCurrentRow();
	if (!r || (str = r->GetValue(field_index, false)) == nullptr)
	{
		return;
	}

	ParseConfigString(*str, obj);
}


#define TABLE_FILLFIELD(name)                                            Table_FillField(tbl, #name, obj.name);
#define TABLE_FILLFIELD_WITH_DEFAULT(name, defaultval)                   Table_FillFieldWithDefault(tbl, #name, obj.name, defaultval)
#define TABLE_FILLFIELD_INDEX(index, name)                               Table_FillField(tbl, (int)index, obj.name);
#define TABLE_FILLFIELD_INDEX_WITH_DEFAULT(index, name, defaultval)      Table_FillFieldWithDefault(tbl, (int)index, obj.name, defaultval)


#endif
