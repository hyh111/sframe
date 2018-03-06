
#ifndef SFRAME_TABLE_H
#define SFRAME_TABLE_H

#include <assert.h>
#include <inttypes.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace sframe {

class Table;

// ±í¸ñÐÐ
class Row
{
public:
	Row(const Row & r) = delete;
	Row & operator=(const Row & r) = delete;

public:
	Row(Table * tbl) : _tbl(tbl) {}
	~Row(){}

	void RemoveColumn(int32_t column_index);

	std::string * GetValue(int32_t column_index, bool new_column);

	std::string * GetValue(const std::string & column_name);

	std::string & operator[](int32_t column_index)
	{
		std::string * v = GetValue(column_index, false);
		assert(v);
		return *v;
	}

private:
	Table * _tbl;
	std::vector<std::string> _data;
};

// ±í¸ñÁÐ
class Column
{
public:
	Column(const Column & r) = delete;
	Column & operator=(const Column & r) = delete;

public:
	Column(Table * tbl, int32_t index) : _tbl(tbl), _index(index) {}

	void SetName(const std::string & name)
	{
		_name = name;
	}

	const std::string & GetName() const
	{
		return _name;
	}

	int32_t GetIndex() const
	{
		return _index;
	}

	std::string * GetValue(int32_t row_index);

	std::string & operator[](int32_t row_index)
	{
		std::string * v = GetValue(row_index);
		assert(v);
		return *v;
	}

private:
	Table * _tbl;
	std::string _name;
	int32_t _index;
};

// ±í¸ñÀà
class Table
{
public:
	Table(const Table & r) = delete;
	Table & operator=(const Table & r) = delete;

public:
	Table() {}

	~Table();

	// Ìí¼ÓÐÂÁÐ
	Column & NewColumn(const std::string & column_name = "");

	// Ìí¼ÓÐÂÐÐ
	Row & NewRow();

	Row & GetRow(int32_t index);

	Column & GetColumn(int32_t index);

	Column * GetColumn(const std::string & name);

	Row & operator[](int32_t index)
	{
		return GetRow(index);
	}

	int32_t GetRowCount() const
	{
		return (int32_t)_rows.size();
	}

	int32_t GetColumnCount() const
	{
		return (int32_t)_columns.size();
	}

	bool RemoveRow(int32_t row_index);

	bool RemoveColumn(int32_t colum_index);

private:
	std::vector<Row*> _rows;           // ËùÓÐÐÐ
	std::vector<Column*> _columns;     // ËùÓÐÁÐ
};

}

#endif
