
#ifndef SFRAME_TABLE_READER_H
#define SFRAME_TABLE_READER_H

#include <assert.h>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <memory>
#include "Table.h"
#include "ConfigStringParser.h"
#include "ConfigLoader.h"
#include "ConfigMeta.h"

namespace sframe {

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

template<typename T_Map>
inline bool Table_FillMap(TableReader & tbl, T_Map & obj)
{
	while (true)
	{
		if (!tbl.GetCurrentRow())
		{
			break;
		}

		typename T_Map::mapped_type v;
		if (!ConfigLoader::Load(tbl, v))
		{
			return false;
		}

		typename T_Map::key_type k = GetConfigObjKey<typename T_Map::key_type>(v);

		if (!PutConfigInContainer::PutInMap(obj, k, v))
		{
			return false;
		}

		tbl.Next();
	}

	return true;
}

template<typename T_Array>
inline bool Table_FillArray(TableReader & tbl, T_Array & obj)
{
	while (true)
	{
		if (!tbl.GetCurrentRow())
		{
			break;
		}

		typename T_Array::value_type v;
		if (!ConfigLoader::Load(tbl, v))
		{
			return false;
		}

		if (!PutConfigInContainer::PutInArray(obj, v))
		{
			return false;
		}

		tbl.Next();
	}

	return true;
}

template<typename T_Set>
inline bool Table_FillSet(TableReader & tbl, T_Set & obj)
{
	while (true)
	{
		if (!tbl.GetCurrentRow())
		{
			break;
		}

		typename T_Set::value_type v;
		if (!ConfigLoader::Load(tbl, v))
		{
			return false;
		}

		if (!PutConfigInContainer::PutInSet(obj, v))
		{
			return false;
		}

		tbl.Next();
	}

	return true;
}

// ÃÓ≥‰TableµΩunorder_map
template<typename T_Key, typename T_Val>
struct ObjectFiller<TableReader, std::unordered_map<T_Key, T_Val>>
{
	static bool Fill(TableReader & tbl, std::unordered_map<T_Key, T_Val> & obj)
	{
		return Table_FillMap(tbl, obj);
	}
};

// ÃÓ≥‰TableµΩmap
template<typename T_Key, typename T_Val>
struct ObjectFiller<TableReader, std::map<T_Key, T_Val>>
{
	static bool Fill(TableReader & tbl, std::map<T_Key, T_Val> & obj)
	{
		return Table_FillMap(tbl, obj);
	}
};

// ÃÓ≥‰TableµΩset
template<typename T>
struct ObjectFiller<TableReader, std::set<T>>
{
	static bool Fill(TableReader & tbl, std::set<T> & obj)
	{
		return Table_FillSet(tbl, obj);
	}
};

// ÃÓ≥‰TableµΩunordered_set
template<typename T>
struct ObjectFiller<TableReader, std::unordered_set<T>>
{
	static bool Fill(TableReader & tbl, std::unordered_set<T> & obj)
	{
		return Table_FillSet(tbl, obj);
	}
};


// ÃÓ≥‰TableµΩvector
template<typename T>
struct ObjectFiller<TableReader, std::vector<T>>
{
	static bool Fill(TableReader & tbl, std::vector<T> & obj)
	{
		return Table_FillArray(tbl, obj);
	}
};

// ÃÓ≥‰TableµΩlist
template<typename T>
struct ObjectFiller<TableReader, std::list<T>>
{
	static bool Fill(TableReader & tbl, std::list<T> & obj)
	{
		return Table_FillArray(tbl, obj);
	}
};

// ÃÓ≥‰TableµΩshared_ptr
template<typename T>
struct ObjectFiller<TableReader, std::shared_ptr<T>>
{
	static bool Fill(TableReader & tbl, std::shared_ptr<T> & obj)
	{
		obj = std::make_shared<T>();
		return ConfigLoader::Load(tbl, *(obj.get()));
	}
};

// ÃÓ≥‰±Ì∏Ò◊÷∂Œ
template<typename T>
inline bool Tbl_FillField(TableReader & reader, const char * field_name, T & obj, const T & default_value = T())
{
	std::string * str = nullptr;
	sframe::Row * r = reader.GetCurrentRow();
	if (!r || (str = r->GetValue(field_name)) == nullptr)
	{
		obj = default_value;
		return false;
	}
	
	ParseCaller::Parse(*str, obj);

	return true;
}

// ÃÓ≥‰±Ì∏Ò◊÷∂Œ
template<typename T>
inline bool Tbl_FillIndex(TableReader & reader, int32_t field_index, T & obj, const T & default_value = T())
{
	std::string * str = nullptr;
	sframe::Row * r = reader.GetCurrentRow();
	if (!r || (str = r->GetValue(field_index, false)) == nullptr)
	{
		obj = default_value;
		return false;
	}

	ParseCaller::Parse(*str, obj);

	return true;
}

}


// ∂‘œÛÃÓ≥‰∏®÷˙∫Í
#define TBL_FILLFIELD(name)                                            sframe::Tbl_FillField(reader, #name, this->name);
#define TBL_FILLFIELD_DEFAULT(name, defaultval)                        sframe::Tbl_FillField(reader, #name, this->name, defaultval)
#define TBL_FILLINDEX(index, name)                                     sframe::Tbl_FillIndex(reader, (int)index, obj.name);
#define TBL_FILLINDEX_DEFAULT(index, name, defaultval)                 sframe::Tbl_FillIndex(reader, (int)index, this->name, defaultval)


#endif
