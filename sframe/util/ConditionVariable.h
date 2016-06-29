
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

class ConditionVariable
{
public:
	ConditionVariable()
	{
		InitializeConditionVariable(&_cond_var);
	}

	~ConditionVariable() {}

	void Wait(AutoLock & lock)
	{
		if (!SleepConditionVariableCS(&_cond_var, &lock.GetLock()->_critical_section, INFINITE))
		{
			assert(false);
		}
	}

	bool Wait(AutoLock & lock, uint32_t milliseconds)
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

	void WakeUpOne()
	{
		WakeConditionVariable(&_cond_var);
	}

	void WakeUpAll()
	{
		WakeAllConditionVariable(&_cond_var);
	}

private:
	CONDITION_VARIABLE _cond_var;
};

#else

class ConditionVariable
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

	void Wait(AutoLock & lock)
	{
		if (pthread_cond_wait(&_cond_var, &lock.GetLock()->_mutex) != 0)
		{
			assert(false);
		}
	}

	bool Wait(AutoLock & lock, uint32_t milliseconds)
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

	void WakeUpOne()
	{
		if (pthread_cond_signal(&_cond_var) != 0)
		{
			assert(false);
		}
	}

	void WakeUpAll()
	{
		if (pthread_cond_broadcast(&_cond_var) != 0)
		{
			assert(false);
		}
	}

private:
	pthread_cond_t _cond_var;
};

#endif

}

#endif
