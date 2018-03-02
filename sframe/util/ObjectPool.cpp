
#include "ObjectPool.h"
#include <assert.h>

using namespace sframe;

FreeMemoryList::FreeMemoryList(int chunk_size, int max_free_list_len)
{
    _max_free_list_len = max_free_list_len;
    _free_list_len = 0;
    _memory_chunk_size = chunk_size > (int32_t)sizeof(ListNode) ? chunk_size : (int32_t)sizeof(ListNode);
    _free_list_header = nullptr;
}

FreeMemoryList::~FreeMemoryList()
{
    void * cur = _free_list_header;
    while (cur)
    {
        void * next = ((ListNode*)cur)->next;
        delete[] (unsigned char*)cur;
        cur = next;
    }
}

// 获取一个内存块
void * FreeMemoryList::GetMemoryChunk()
{
    AUTO_LOCK(_locker);

    void * chunk = nullptr;

    // 从空闲链表中取
    if (_free_list_header != nullptr)
    {
        chunk = _free_list_header;
        _free_list_header = ((ListNode*)_free_list_header)->next;
        _free_list_len--;
        assert(_free_list_len >= 0);
    }

    // 没有取到，new一块新内存
    if (chunk == nullptr)
    {
        chunk = new unsigned char[_memory_chunk_size];
    }

    return chunk;
}

// 释放内存块
void FreeMemoryList::FreeMemoryChunk(void * memory_chunk)
{
    if (memory_chunk == nullptr)
        return;

    AUTO_LOCK(_locker);

    if (_free_list_len < _max_free_list_len)
    {
        // 将内存块插入到头部
        ((ListNode*)memory_chunk)->next = _free_list_header;
        _free_list_header = memory_chunk;
        _free_list_len++;
    }
    else
    {
        delete[] (unsigned char*)memory_chunk;
    }
}