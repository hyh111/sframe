
#ifndef SFRAME_LOCK_H
#define SFRAME_LOCK_H

#include <atomic>

#ifndef __GNUC__
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace sframe {

#ifndef __GNUC__
// Windows锁实现
class Lock
{
public:
    Lock()
    {
        InitializeCriticalSection(&this->_critical_section);
    }
    ~Lock()
    {
        DeleteCriticalSection(&this->_critical_section);
    }

    void lock()
    {
        EnterCriticalSection(&this->_critical_section);
    }

    void unlock()
    {
        LeaveCriticalSection(&this->_critical_section);
    }

private:
    CRITICAL_SECTION _critical_section;
};

#else

// Linux锁实现
class Lock
{
public:
    Lock()
    {
        _mutex = PTHREAD_MUTEX_INITIALIZER;
    }

    ~Lock()
    {}

    void lock()
    {
        pthread_mutex_lock(&_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }
private:
    pthread_mutex_t _mutex;
};

#endif

// 自旋锁
class SpinLock
{
public:
	SpinLock()
	{
		_lock.clear(std::memory_order_relaxed);
	}

	void lock()
	{
		while (_lock.test_and_set(std::memory_order_acquire)) {}
	}

	inline bool try_lock()
	{
		return (!_lock.test_and_set(std::memory_order_acquire));
	}

	void unlock()
	{
		_lock.clear(std::memory_order_release);
	}

private:
	std::atomic_flag _lock;
};


template<typename T_Lock>
class AutoLock
{
public:
    AutoLock(T_Lock & lock) : _lock(&lock)
    {
        _lock->lock();
    }

    ~AutoLock()
    {
        _lock->unlock();
    }

private:
	T_Lock * _lock;
};

}

#endif