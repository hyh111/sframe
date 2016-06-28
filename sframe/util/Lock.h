
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

class AutoLock
{
public:
    AutoLock(Lock & lock) : _lock(&lock)
    {
        _lock->lock();
    }

    ~AutoLock()
    {
        _lock->unlock();
    }

private:
	Lock * _lock;
};

#define AUTO_LOCK(_lc) sframe::AutoLock l((_lc))

}

#endif