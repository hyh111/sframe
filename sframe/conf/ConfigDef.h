
#ifndef SFRAME_CONFIG_DEF_H
#define SFRAME_CONFIG_DEF_H

namespace json11 { class Json; }

template<typename T_Config>
struct ConfigInfoHelper
{
};

#define JSON_OBJECT(S) \
	struct S; \
	void Json_FillObject(const json11::Json & json, S & obj); \
	struct S

#define JSON_CONFIG(S, file_name, config_id) \
	struct S; \
	template<> \
	struct ConfigInfoHelper<S> \
	{ \
		static const char * GetFileName() {return file_name;} \
		static int GetConfigId() {return config_id;}\
	};\
	void Json_FillObject(const json11::Json & json, S & obj); \
	struct S

class TableReader;

#define TABLE_OBJECT(S) \
	struct S; \
	void Table_FillObject(TableReader & tbl, S & obj); \
	struct S

#define TABLE_CONFIG(S, file_name, config_id) \
	struct S; \
	template<> \
	struct ConfigInfoHelper<S> \
	{ \
		static const char * GetFileName() {return file_name;} \
		static int GetConfigId() {return config_id;}\
	};\
	void Table_FillObject(TableReader & tbl, S & obj); \
	struct S


#define GET_CONFIGID(T) ConfigInfoHelper<T>::GetConfigId()

#define GET_CONFIGFILENAME(T) ConfigInfoHelper<T>::GetFileName()

#endif