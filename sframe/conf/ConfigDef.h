
#ifndef SFRAME_CONFIG_DEF_H
#define SFRAME_CONFIG_DEF_H

namespace json11 { class Json; }

#define JSONCONFIG(S) \
	struct S; \
	void Json_FillObject(const json11::Json & json, S & obj); \
	struct S

#define FILL_JSONCONFIG(S) void Json_FillObject(const json11::Json & json, S & obj)

class TableReader;

#define TABLECONFIG(S) \
	struct S; \
	void Table_FillObject(TableReader & tbl, S & obj); \
	struct S

#define FILL_TABLECONFIG(S) void Table_FillObject(TableReader & tbl, S & obj)

#define CONFIGINFO(id, filename) \
	static const char * GetFileName() {return filename;} \
	static const int kConfigId = id

#define GET_CONFIGID(T) T::kConfigId

#define GET_CONFIGFILENAME(T) T::GetFileName()

#endif