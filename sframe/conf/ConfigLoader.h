
#ifndef __CONFIG_LOADER_H__
#define __CONFIG_LOADER_H__

#include <inttypes.h>
#include <assert.h>

namespace sframe {

template<typename T_Reader, typename T_Obj>
struct ObjectFiller
{
	static bool Fill(T_Reader & reader, T_Obj & obj)
	{
		assert(false);
		return false;
	}
};

// 配置加载器
struct ConfigLoader
{
	template<typename T_Reader, typename T_Obj, int>
	struct Caller
	{
		static bool Call(T_Reader & reader, T_Obj & obj)
		{
			return ObjectFiller<T_Reader, T_Obj>::Fill(reader, obj);
		}
	};

	template<typename T_Reader, typename T_Obj>
	struct Caller<T_Reader, T_Obj, 1>
	{
		static bool Call(T_Reader & reader, T_Obj & obj)
		{
			return obj.Fill(reader);
		}
	};

	template<typename T_Reader, typename T_Obj>
	struct Caller<T_Reader, T_Obj, 2>
	{
		static bool Call(T_Reader & reader, T_Obj & obj)
		{
			obj.Fill(reader);
			return true;
		}
	};

	template<typename T_Reader, typename T_Obj>
	struct LoaderType
	{
		// 匹配器 ———— bool返回值类成员函数，形如 bool T_Obj::FillObject(T_Reader & reader)
		template<typename U, bool(U::*)(T_Reader &)>
		struct MethodMatcher_MemeberFuncWithBoolReturn;

		// 匹配器 ———— 无返回值类成员函数，形如 void T_Obj::FillObject(T_Reader & reader)
		template<typename U, void(U::*)(T_Reader &)>
		struct MethodMatcher_MemeberFuncWithNoReturn;

		template<typename U>
		static int8_t match(MethodMatcher_MemeberFuncWithBoolReturn<U, &U::Fill>*);

		template<typename U>
		static int16_t match(MethodMatcher_MemeberFuncWithNoReturn<U, &U::Fill>*);

		template<typename U>
		static int64_t match(...);

		static const int value = sizeof(match<T_Obj>(NULL));
	};

	template<typename T_Reader, typename T_Obj>
	static bool Load(T_Reader & reader, T_Obj & obj)
	{
		return Caller<T_Reader, T_Obj, LoaderType<T_Reader, T_Obj>::value>::Call(reader, obj);
	}

};


// 配置初始化器
struct ConfigInitializer
{
	template<typename T, int, typename... T_Args>
	struct Caller
	{
		static bool Call(T_Args & ... args)
		{
			return true;
		}
	};

	template<typename T, typename... T_Args>
	struct Caller<T, 1, T_Args...>
	{
		static bool Call(T_Args & ... args)
		{
			return T::Initialize(args...);
		}
	};

	template<typename T, typename... T_Args>
	struct Caller<T, 2, T_Args...>
	{
		static bool Call(T_Args & ... args)
		{
			T::Initialize(args...);
			return true;
		}
	};

	template<typename T, typename... T_Args>
	struct InitializerType
	{
		// 匹配器——带返回值的初始化方法
		template<bool(*)(T_Args & ...)>
		struct MethodMatcher_WithReturnedValue;

		// 匹配器——带返回值的初始化方法
		template<void(*)(T_Args & ...)>
		struct MethodMatcher_WithNoReturnedValue;

		template<typename U>
		static int8_t match(MethodMatcher_WithReturnedValue<&U::Initialize>*);

		template<typename U>
		static int16_t match(MethodMatcher_WithNoReturnedValue<&U::Initialize>*);

		template<typename U>
		static int32_t match(...);

		// 1 带返回值的初始化方法
		// 2 不带返回值得初始化方法
		// 4 没有初始化方法
		static const int value = sizeof(match<T>(NULL));
	};

	template<typename T, typename... T_Args>
	static bool Initialize(T_Args & ... args)
	{
		return Caller<T, InitializerType<T, T_Args...>::value, T_Args...>::Call(args...);
	}

};

}

#endif