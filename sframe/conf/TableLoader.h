
#ifndef SFRAME_TABLE_LOADER_H
#define SFRAME_TABLE_LOADER_H

#include "../util/FileHelper.h"
#include "../util/Log.h"
#include "TableReader.h"

namespace sframe {

// 表格读取器
template<typename T_Parser>
class TableLoader
{
	enum 
	{
		kHeaderRow_Declare,         // 申明
		kHeaderRow_ColumnName,      // 列名
		kHeaderRow_ColumnExplain,   // 注释
		kHeaderRow_ColumnType,      // 类型

		kHeaderRow_Count
	};

public:
	template<typename T>
	static bool Load(const std::string & full_name, T & o)
	{
		std::string content;
		if (!FileHelper::ReadFile(full_name, content))
		{
			LOG_ERROR << "Cannot open config file(" << full_name << ")" << ENDL;
			return false;
		}

		Table tbl;
		if (!T_Parser::Parse(content, tbl))
		{
			LOG_ERROR << "Config file(" << full_name << ") format error" << ENDL;
			return false;
		}

		if (tbl.GetRowCount() < kHeaderRow_Count)
		{
			LOG_ERROR << "Config file(" << full_name << ") header error" << ENDL;
			return false;
		}

		// 设置列名
		int32_t column_count = tbl.GetColumnCount();
		for (int i = 0; i < column_count; i++)
		{
			tbl.GetColumn(i).SetName(tbl[kHeaderRow_ColumnName][i]);
		}

		// 删除头部
		int32_t surplus_header = kHeaderRow_Count;
		while (surplus_header > 0)
		{
			tbl.RemoveRow(0);
			surplus_header--;
		}

		TableReader reader(tbl);
		return ConfigLoader::Load(reader, o);
	}
};

}

#endif
