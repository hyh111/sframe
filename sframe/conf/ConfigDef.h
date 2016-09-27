
#ifndef SFRAME_CONFIG_DEF_H
#define SFRAME_CONFIG_DEF_H

#include "ConfigSet.h"
#include "TableReader.h"
#include "JsonReader.h"

//////////////////// 静态配置声明相关辅助

// 声明单一对象模型的配置
// STATIC_OBJ_CONFIG(结构体名, 配置ID)
#define STATIC_OBJ_CONFIG(S, conf_id) \
	struct ConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef S ModelType; \
	};

// 声明Vetor模型的配置
// STATIC_VECTOR_CONFIG(结构体名, 配置ID)
#define STATIC_VECTOR_CONFIG(S, conf_id) \
	struct ConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef std::vector<S> ModelType; \
	};

// 声明Set模型的配置
// STATIC_SET_CONFIG(结构体名, 配置ID)
#define STATIC_SET_CONFIG(S, conf_id) \
	struct ConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef std::set<S> ModelType; \
	};

// 声明Map模型的配置
// STATIC_MAP_CONFIG(key类型, 结构体名, 结构体中用作key的成员变量名, 配置ID)
#define STATIC_MAP_CONFIG(k_type, S, k_field_name, conf_id) \
	struct ConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef k_type KeyType; \
		typedef std::map<k_type, std::shared_ptr<S>> ModelType; \
		static k_type GetKey(const S & conf) { return conf.k_field_name; } \
	};

// 对象填充辅助宏
#define FILLFIELD(name)                                            sframe::FillField(reader, #name, this->name);
#define FILLFIELD_DEFAULT(name, defaultval)                        sframe::FillField(reader, #name, this->name, defaultval)
#define FILLINDEX(index, name)                                     sframe::FillIndex(reader, (int)index, obj.name);
#define FILLINDEX_DEFAULT(index, name, defaultval)                 sframe::FillIndex(reader, (int)index, this->name, defaultval)



//////////////////// 动态配置申明相关辅助

// 声明单一对象模型的配置
// DYNAMIC_OBJ_CONFIG(结构体名, 配置ID)
#define DYNAMIC_OBJ_CONFIG(S, conf_id) \
	struct DynamicConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef S ModelType; \
	};

// 声明Vetor模型的配置
// DYNAMIC_VECTOR_CONFIG(结构体名, 配置ID)
#define DYNAMIC_VECTOR_CONFIG(S, conf_id) \
	struct DynamicConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef std::vector<S> ModelType; \
	};

// 声明Set模型的配置
// DYNAMIC_SET_CONFIG(结构体名, 配置ID)
#define DYNAMIC_SET_CONFIG(S, conf_id) \
	struct DynamicConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef std::set<S> ModelType; \
	};

// 声明Map模型的配置
// DYNAMIC_MAP_CONFIG(key类型, 结构体名, 结构体中用作key的成员变量名, 配置ID)
#define DYNAMIC_MAP_CONFIG(k_type, S, k_field_name, conf_id) \
	struct DynamicConfigMeta \
	{ \
		static int GetConfigId() {return conf_id;} \
		typedef k_type KeyType; \
		typedef std::map<k_type, std::shared_ptr<S>> ModelType; \
		static k_type GetKey(const S & conf) { return conf.k_field_name; } \
	};

#endif
