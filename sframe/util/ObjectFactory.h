
#ifndef SFRAME_OBJECT_FACTORY_H
#define SFRAME_OBJECT_FACTORY_H

#include <assert.h>
#include <inttypes.h>
#include <typeinfo>
#include <string>
#include <unordered_map>
#include "Singleton.h"

namespace sframe {

// 动态对象基类
class DynamicObject
{
public:
	DynamicObject() {}
	virtual ~DynamicObject() {}
};

typedef DynamicObject* (*Func_CreateObject)();

// 对象工厂
template<typename T_ObjKey>
class ObjectFactory
{
public:
	typedef T_ObjKey KeyType;

	virtual ~ObjectFactory() {}

	bool Regist(const T_ObjKey & key, Func_CreateObject func)
	{
		if (!func)
		{
			return false;
		}

		return _creator_map.insert(std::make_pair(key, func)).second;
	}

	template<typename T>
	T * Create(const T_ObjKey & key)
	{
		DynamicObject * obj = CreateDynamicObject(key);
		if (!obj)
		{
			return nullptr;
		}
		T * real_obj = dynamic_cast<T*>(obj);
		if (!real_obj)
		{
			delete obj;
			return nullptr;
		}
		return real_obj;
	}

private:
	DynamicObject * CreateDynamicObject(const T_ObjKey & key)
	{
		auto it = _creator_map.find(key);
		if (it == _creator_map.end())
		{
			return nullptr;
		}

		return it->second();
	}

private:
	std::unordered_map<T_ObjKey, Func_CreateObject> _creator_map;
};

// 全局对象工厂
class GlobalObjFactory : public ObjectFactory<std::string>, public singleton<GlobalObjFactory> {};


// 注册到工厂
// 工厂必须定义了单例方法 static Factory & Factory::Instance()
template<typename T, typename T_Factory, typename T_Factory::KeyType key>
class RegFactory : public DynamicObject
{
private:
	static DynamicObject * CreateObject()
	{
		return new T();
	}

	struct Registor
	{
		Registor()
		{
			if (!T_Factory::Instance().Regist(key, CreateObject))
			{
				assert(false);
			}
		}

		inline void do_nothing()const {}
	};

	static Registor s_registor;

public:
	RegFactory()
	{
		s_registor.do_nothing();
	}

	virtual ~RegFactory()
	{
		s_registor.do_nothing();
	}
};

template <typename T, typename T_Factory, typename T_Factory::KeyType key>
typename RegFactory<T, T_Factory, key>::Registor RegFactory<T, T_Factory, key>::s_registor;


// 解析类型名称（转换为 A::B::C 的形式）
std::string ReadTypeName(const char * name);

// 注册到工厂，通过类名
// 工厂必须定义了单例方法 static Factory & Factory::Instance()
template<typename T, typename T_Factory = GlobalObjFactory>
class RegFactoryByName : public DynamicObject
{
private:
	static DynamicObject * CreateObject()
	{
		return new T();
	}

	struct Registor
	{
		Registor()
		{
			std::string name = ReadTypeName(typeid(T).name());
			if (!T_Factory::Instance().Regist(name, CreateObject))
			{
				assert(false);
			}
		}

		inline void do_nothing()const {}
	};

public:
	static Registor s_registor;

	RegFactoryByName()
	{
		s_registor.do_nothing();
	}

	virtual ~RegFactoryByName()
	{
		s_registor.do_nothing();
	}
};

template <typename T, typename T_Factory>
typename RegFactoryByName<T, T_Factory>::Registor RegFactoryByName<T, T_Factory>::s_registor;

}

#endif