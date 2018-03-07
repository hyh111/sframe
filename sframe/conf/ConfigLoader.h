
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
		static bool Call(T & obj, T_Args & ... args)
		{
			return true;
		}
	};

	template<typename T, typename... T_Args>
	struct Caller<T, 1, T_Args...>
	{
		static bool Call(T & obj, T_Args & ... args)
		{
			return obj.Initialize(args...);
		}
	};

	template<typename T, typename... T_Args>
	struct Caller<T, 2, T_Args...>
	{
		static bool Call(T & obj, T_Args & ... args)
		{
			obj.Initialize(args...);
			return true;
		}
	};

	template<typename T, typename... T_Args>
	struct InitializerType
	{
		// 匹配器——带返回值的初始化方法
		template<bool(T::*)(T_Args & ...)>
		struct MethodMatcher_WithReturnedValue;

		// 匹配器——带返回值的初始化方法
		template<void(T::*)(T_Args & ...)>
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
	static bool Initialize(T & obj, T_Args & ... args)
	{
		return Caller<T, InitializerType<T, T_Args...>::value, T_Args...>::Call(obj, args...);
	}

};

// 将配置对象放入容器
struct PutConfigInContainer
{
	template<typename T_Map, typename T_Key, typename T_Obj, bool>
	struct PutInMapCaller
	{
		static bool Put(T_Map & m, T_Key & key, T_Obj & obj)
		{
			return m.insert(std::make_pair(key, obj)).second;
		}
	};

	template<typename T_Map, typename T_Key, typename T_Obj>
	struct PutInMapCaller<T_Map, T_Key, T_Obj, true>
	{
		static bool Put(T_Map & m, T_Key &, T_Obj & obj)
		{
			return obj.PutIn(m);
		}
	};

	template<typename T_Map, typename T_Key, typename T_Obj>
	struct PutInMapCaller<T_Map, T_Key, std::shared_ptr<T_Obj>, true>
	{
		static bool Put(T_Map & m, T_Key & k, std::shared_ptr<T_Obj> & obj)
		{
			return PutInMapCaller<T_Map, T_Key, T_Obj, true>::Put(m, k, *obj.get());
		}
	};

	template<typename T_Set, typename T_Obj, bool>
	struct PutInSetCaller
	{
		static bool Put(T_Set & m, T_Obj & obj)
		{
			return m.insert(obj).second;
		}
	};

	template<typename T_Set, typename T_Obj>
	struct PutInSetCaller<T_Set, T_Obj, true>
	{
		static bool Put(T_Set & m, T_Obj & obj)
		{
			return obj.PutIn(m);
		}
	};

	template<typename T_Set, typename T_Obj>
	struct PutInSetCaller<T_Set, std::shared_ptr<T_Obj>, true>
	{
		static bool Put(T_Set & m, std::shared_ptr<T_Obj> & obj)
		{
			return PutInSetCaller<T_Set, T_Obj, true>::Put(m, *obj.get());
		}
	};

	template<typename T_Array, typename T_Obj, bool>
	struct PutInArrayCaller
	{
		static bool Put(T_Array & arr, T_Obj & obj)
		{
			arr.push_back(obj);
			return true;
		}
	};

	template<typename T_Array, typename T_Obj>
	struct PutInArrayCaller<T_Array, T_Obj, true>
	{
		static bool Put(T_Array & arr, T_Obj & obj)
		{
			return obj.PutIn(arr);
		}
	};

	template<typename T_Array, typename T_Obj>
	struct PutInArrayCaller<T_Array, std::shared_ptr<T_Obj>, true>
	{
		static bool Put(T_Array & arr, std::shared_ptr<T_Obj> & obj)
		{
			return PutInArrayCaller<T_Array, T_Obj, true>::Put(arr, *obj.get());
		}
	};

	template<typename T_Container, typename T_Obj>
	struct HaveMethod
	{
		// 匹配器 ———— 形如 bool T_Obj::FillObject(T_Reader & reader)
		template<typename U, bool(U::*)(T_Container &)>
		struct MethodMatcher;

		template<typename U>
		static int8_t match(MethodMatcher<U, &U::PutIn>*);

		template<typename U>
		static int32_t match(...);

		template<typename U>
		struct GetRealObjType
		{
			typedef U RealObjType;
		};

		template<typename U>
		struct GetRealObjType<std::shared_ptr<U>>
		{
			typedef typename GetRealObjType<U>::RealObjType RealObjType;
		};

		static const bool value = (sizeof(match<typename GetRealObjType<T_Obj>::RealObjType>(NULL)) == 1);
	};

	template<typename T_Map, typename T_Key, typename T_Obj>
	static bool PutInMap(T_Map & m, T_Key & key, T_Obj & obj)
	{
		return PutInMapCaller<T_Map, T_Key, T_Obj, HaveMethod<T_Map, T_Obj>::value>::Put(m, key, obj);
	}

	template<typename T_Array, typename T_Obj>
	static bool PutInArray(T_Array & arr, T_Obj & obj)
	{
		return PutInArrayCaller<T_Array, T_Obj, HaveMethod<T_Array, T_Obj>::value>::Put(arr, obj);
	}

	template<typename T_Set, typename T_Obj>
	static bool PutInSet(T_Set & s, T_Obj & obj)
	{
		return PutInSetCaller<T_Set, T_Obj, HaveMethod<T_Set, T_Obj>::value>::Put(s, obj);
	}
};

}

#endif