
#ifndef SFRAME_OBJECT_POOL_H
#define SFRAME_OBJECT_POOL_H

#include <inttypes.h>
#include <new>
#include <utility>
#include <type_traits>
#include "Lock.h"
#include "Singleton.h"

namespace sframe{

// 空闲内存链表
class FreeMemoryList
{
private:

    struct ListNode
    {
        void * next;
    };

public:

    FreeMemoryList(int chunk_size, int max_free_list_len);

    ~FreeMemoryList();

    // 获取一个内存块
    void * GetMemoryChunk();

    // 释放内存块
    void FreeMemoryChunk(void * memory_chunk);

private:
    int _memory_chunk_size;   // 内存快大小
    int _max_free_list_len;  // 最大空闲内存快数量
    int _free_list_len;
    void * _free_list_header; // 空闲内存链表头
    Lock _locker;
};

class MaxObjectPoolSize
{
	template<int32_t(*)()>
	struct MethodMatcher;

	template<typename T>
	static std::true_type match(MethodMatcher<&T::GetObjectPoolSize>*) { return std::true_type(); }

	template<typename T>
	static std::false_type match(...) { return std::false_type(); }

	template<typename T>
	inline static int32_t call(std::false_type)
	{
		return 1024 * 4;
	}

	template<typename T>
	inline static int32_t call(std::true_type)
	{
		return T::GetObjectPoolSize();
	}

public:
	template<typename T>
	inline static int32_t GetSize()
	{
		return call<T>(decltype(match<T>(nullptr))());
	}
};

// 通用对象池
template<typename T>
class ObjectPool : public singleton<ObjectPool<T>>
{
public:
    ObjectPool() : _free_memory_list(sizeof(T), MaxObjectPoolSize::GetSize<T>()) {}

	// 创建对象
	template<typename... T_Args>
	T * New(T_Args&... args)
	{
		void * mem = _free_memory_list.GetMemoryChunk();
		T * obj = new(mem)T(std::forward<T_Args>(args)...);
		return obj;
	}

    // 释放对象
    void Delete(T * obj)
    {
        if (obj == nullptr)
            return;
        obj->~T();
        _free_memory_list.FreeMemoryChunk((char*)obj);
    }

private:
    FreeMemoryList _free_memory_list;  // 空闲内存链表
};

}

#endif