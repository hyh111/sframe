
#include <assert.h>
#include "Table.h"

using namespace sframe;

void Row::RemoveColumn(int32_t column_index)
{
	if (column_index < 0 || column_index >= (int32_t)_data.size())
	{
		return;
	}

	_data.erase(_data.begin() + column_index);
}

std::string * Row::GetValue(int32_t column_index, bool new_column)
{
	int32_t column_count = _tbl->GetColumnCount();
	if (column_index >= column_count && new_column)
	{
		int32_t add_column_count = column_index - column_count + 1;
		while (add_column_count > 0)
		{
			_tbl->NewColumn();
			add_column_count--;
			column_count++;
		}
	}

	int32_t cur_column_count = (int32_t)_data.size();
	int32_t new_count = column_count - cur_column_count;
	if (new_count > 0)
	{
		_data.insert(_data.end(), new_count, "");
	}

	if (column_index >= 0 && column_index < column_count)
	{
		return &_data[column_index];
	}

	return nullptr;
}

std::string * Row::GetValue(const std::string & column_name)
{
	Column * col = _tbl->GetColumn(column_name);
	if (!col)
	{
		return nullptr;
	}

	return GetValue(col->GetIndex(), false);
}


std::string * Column::GetValue(int32_t row_index)
{
	if (row_index >= _tbl->GetRowCount())
	{
		return nullptr;
	}

	return &(*_tbl)[row_index][_index];
}


Table::~Table()
{
	for (auto it : _rows)
	{
		delete it;
	}

	for (auto it : _columns)
	{
		delete it;
	}
}

// 添加新列
Column & Table::NewColumn(const std::string & column_name)
{
	int32_t new_index = (int32_t)_columns.size();
	Column * col = new Column(this, new_index);
	assert(col);

	if (!column_name.empty())
	{
		col->SetName(column_name);
	}

	_columns.push_back(col);

	return *col;
}

// 添加新行
Row & Table::NewRow()
{
	Row * r = new Row(this);
	assert(r);
	_rows.push_back(r);

	return *r;
}

Row & Table::GetRow(int32_t index)
{
	assert(index >= 0 && index < GetRowCount());
	return *_rows[index];
}

Column & Table::GetColumn(int32_t index)
{
	assert(index >= 0 && index < GetColumnCount());
	return *_columns[index];
}

Column * Table::GetColumn(const std::string & name)
{
	Column * col = nullptr;

	for (auto c : _columns)
	{
		if (c->GetName() == name)
		{
			col = c;
			break;
		}
	}

	return col;
}

bool Table::RemoveRow(int32_t row_index)
{
	if (row_index < 0 || row_index >= GetRowCount())
	{
		return false;
	}

	delete _rows[row_index];
	_rows.erase(_rows.begin() + row_index);

	return true;
}

bool Table::RemoveColumn(int32_t colum_index)
{
	if (colum_index < 0 || colum_index >= GetColumnCount())
	{
		return false;
	}

	delete _columns[colum_index];
	_columns.erase(_columns.begin() + colum_index);

	for (auto row : _rows)
	{
		row->RemoveColumn(colum_index);
	}

	return true;
}
