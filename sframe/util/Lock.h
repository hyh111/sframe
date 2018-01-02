
#ifndef SFRAME_LOCK_H
#define SFRAME_LOCK_H

#include <assert.h>
#ifndef __GNUC__
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "Singleton.h"

namespace sframe {

#ifndef __GNUC__
// Windows锁实现
class Lock : public noncopyable
{
	friend class ConditionVariable;
public:
    Lock()
    {
        InitializeCriticalSection(&this->_critical_section);
    }
    ~Lock()
    {
        DeleteCriticalSection(&this->_critical_section);
    }

    void lock() const
    {
        EnterCriticalSection(&this->_critical_section);
    }

    void unlock() const
    {
        LeaveCriticalSection(&this->_critical_section);
    }

private:
    mutable CRITICAL_SECTION _critical_section;
};

#else

// Linux锁实现
class Lock : public noncopyable
{
	friend class ConditionVariable;
public:
    Lock() : _mutex(PTHREAD_MUTEX_INITIALIZER) {}

    ~Lock() 
	{
		if (pthread_mutex_destroy(&_mutex) != 0)
		{
			assert(false);
		}
	}

    void lock() const
    {
        pthread_mutex_lock(&_mutex);
    }

    void unlock() const
    {
        pthread_mutex_unlock(&_mutex);
    }
private:
	mutable pthread_mutex_t _mutex;
};

#endif

class AutoLock : public noncopyable
{
public:
    AutoLock(const Lock & lock) : _lock(&lock)
    {
        _lock->lock();
    }

    ~AutoLock()
    {
        _lock->unlock();
    }

	const Lock * GetLock() const
	{
		return _lock;
	}

private:
	const Lock * _lock;
};

#define AUTO_LOCK(_lc) sframe::AutoLock l((_lc))

}

#endif