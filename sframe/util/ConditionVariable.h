
#ifndef SFRAME_CONDITION_VARIABLE_H
#define SFRAME_CONDITION_VARIABLE_H

#include <inttypes.h>
#include "Lock.h"

#ifndef __GNUC__
#include <windows.h>
#else
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#endif

namespace sframe {

#ifndef __GNUC__

class ConditionVariable : public noncopyable
{
public:
	ConditionVariable()
	{
		InitializeConditionVariable(&_cond_var);
	}

	~ConditionVariable() {}

	void Wait(const AutoLock & lock) const
	{
		if (!SleepConditionVariableCS(&_cond_var, &lock.GetLock()->_critical_section, INFINITE))
		{
			assert(false);
		}
	}

	bool Wait(const AutoLock & lock, uint32_t milliseconds) const
	{
		if (!SleepConditionVariableCS(&_cond_var, &lock.GetLock()->_critical_section, (DWORD)milliseconds))
		{
			DWORD err_code = GetLastError();
			if (err_code != ERROR_TIMEOUT)
			{
				assert(false);
			}
			return false;
		}

		return true;
	}

	void WakeUpOne() const
	{
		WakeConditionVariable(&_cond_var);
	}

	void WakeUpAll() const
	{
		WakeAllConditionVariable(&_cond_var);
	}

private:
	mutable CONDITION_VARIABLE _cond_var;
};

#else

class ConditionVariable : public noncopyable
{
public:
	ConditionVariable() : _cond_var(PTHREAD_COND_INITIALIZER) {}

	~ConditionVariable()
	{
		if (pthread_cond_destroy(&_cond_var) != 0)
		{
			assert(false);
		}
	}

	void Wait(const AutoLock & lock) const
	{
		if (pthread_cond_wait(&_cond_var, &lock.GetLock()->_mutex) != 0)
		{
			assert(false);
		}
	}

	bool Wait(const AutoLock & lock, uint32_t milliseconds) const
	{
		timeval now;
		gettimeofday(&now, NULL);
		int64_t nsec = ((int64_t)now.tv_usec + (int64_t)milliseconds * 1000) * 1000;

		timespec abstime;
		abstime.tv_sec += now.tv_sec + (__time_t)(nsec / 1000000000);
		abstime.tv_nsec = (nsec % 1000000000);

		int ret = pthread_cond_timedwait(&_cond_var, &lock.GetLock()->_mutex, &abstime);
		if (ret != 0)
		{
			if (ret != ETIMEDOUT)
			{
				assert(false);
			}
			return false;
		}

		return true;
	}

	void WakeUpOne() const
	{
		if (pthread_cond_signal(&_cond_var) != 0)
		{
			assert(false);
		}
	}

	void WakeUpAll() const
	{
		if (pthread_cond_broadcast(&_cond_var) != 0)
		{
			assert(false);
		}
	}

private:
	mutable pthread_cond_t _cond_var;
};

#endif

}

#endif
