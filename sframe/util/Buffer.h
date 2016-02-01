
#ifndef SFRAME_BUFFER_H
#define SFRAME_BUFFER_H

#include <inttypes.h>
#include <assert.h>
#include <atomic>
#include <algorithm>

namespace sframe{


// 环形缓冲区(同一时间只能有一个线程读)
template<typename T, int32_t Buffer_Capacity>
class FastRingBuffer
{
public:
    FastRingBuffer()
    {
        static_assert(std::is_pod<T>::value, "FastRingBuffer T must be pod type");
        static_assert(Buffer_Capacity > 0 && Buffer_Capacity % 2 == 0, "This is a bad FastRingBuffer capacity");
        _idle.store(Buffer_Capacity);
        _apply.store(0);
        _cursor.store(0);
        _readed = 0;
    }

    // 压入
    bool Push(const T & obj)
    {
        // 首先占有需要的空闲长度，并比较空闲的长度是否足够
        if (_idle.fetch_sub(1) < 1)
        {
            _idle.fetch_add(1);
            return false;
        }

        // 申请一个位置
        uint64_t apply_num = _apply.fetch_add(1);
        // 写入位置
        int32_t write_index = (int32_t)(apply_num & (Buffer_Capacity - 1));
        // 写入
        _buf[write_index] = obj;

        // 提交写入的数据（使用CAS修改 _cursor）
        while (true)
        {
            auto tmp = apply_num;
            if (_cursor.compare_exchange_weak(tmp, apply_num + 1))
            {
                break;
            }
        }

        return true;
    }

    // 压入
    bool Push(const T * data, int32_t len)
    {
        if (data == nullptr || len <= 0)
        {
            return false;
        }
    
        // 首先占有需要的空闲长度，并比较空闲的长度是否足够
        if (_idle.fetch_sub(len) < len)
        {
            _idle.fetch_add(len);
            return false;
        }
    
        // 申请一块缓冲区
        uint64_t before_apply = _apply.fetch_add((uint32_t)len);
        uint64_t after_apply = before_apply + len;
        // 计算写入位置
        int32_t begin_write_index = (int32_t)(before_apply & (Buffer_Capacity - 1));
        int32_t end_write_index = (int32_t)(after_apply & (Buffer_Capacity - 1));
    
        // 不需分两次写入
        if (end_write_index > begin_write_index)
        {
            memcpy(_buf + begin_write_index, data, len);
        }
        // 需要分两次写入
        else
        {
            int32_t first_write_len = Buffer_Capacity - begin_write_index;
            memcpy(_buf + begin_write_index, data, first_write_len);
            memcpy(_buf, data + first_write_len, len - first_write_len);
        }
    
        // 提交写入的数据（循环CAS修改_cursor）
        while (true)
        {
            auto tmp = before_apply;
            if (_cursor.compare_exchange_weak(tmp, after_apply))
            {
                break;
            }
        }
    
        return true;
    }

    // 读取数据，读取后，缓冲区依然锁定(不空闲)
    T * Peek(int32_t & len)
    {
        uint64_t cursor = _cursor.load();

        if (_readed >= cursor)
        {
            len = 0;
            return nullptr;
        }

        // 可读数据
        int32_t can_read = (int32_t)(cursor - _readed);
        // 读取开始索引
        int32_t begin_index = (int32_t)(_readed & (Buffer_Capacity - 1));
        int32_t end_index = (int32_t)(cursor & (Buffer_Capacity - 1));

        T * ret_buf = nullptr;

        if (end_index <= begin_index)
        {
            // 只返回第一段数据
            can_read = Buffer_Capacity - begin_index;
        }

        len = (len > 0 && len < can_read) ? len : can_read;
        _readed += len;
        ret_buf = _buf + begin_index;

        return ret_buf;
    }

    // 加读取游标
    void AddRead(int32_t val)
    {
        _readed += val;
    }

    // 获取未读取的长度
    int32_t GetUnReadLength() const
    {
        uint64_t cursor = _cursor.load();
        return (int32_t)(cursor - _readed);
    }

    // 将指定缓冲区变为空闲状态，使生产者可以写入
    void Free(int32_t len)
    {
        int64_t idle = _idle.load();
        while (idle < 0)
        {
            idle = _idle.load();
        }

        assert(len + idle <= Buffer_Capacity);

        _idle.fetch_add(len);
    }

private:
    T _buf[Buffer_Capacity];
    std::atomic_llong _idle;      // 空闲的长度（为了保证未处理的数据不会备覆盖）
    std::atomic_ullong _apply;    // 申请缓冲区的游标（生产者申请区域的记录）
    std::atomic_ullong _cursor;   // 数据游标（指示可用数据）
    uint64_t _readed;            // 读取游标，指示当前读取位置
};

}

#endif