
#ifndef SFRAME_TIMEHELPER_H
#define SFRAME_TIMEHELPER_H

#include <inttypes.h>
#include <time.h>
#include <chrono>

#ifndef __GNUC__
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace sframe{

// 事件帮助
class TimeHelper
{
public:
    // 系统启动到现在的时间（微秒）
    static int64_t GetSystenTickCount()
    {
#ifndef __GNUC__
        return (int64_t)GetTickCount64();
#else
        return 0;
#endif
    }

    // 纪元到现在的毫秒数
    static int64_t GetEpochMilliseconds()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // 纪元到现在的秒数
    static int64_t GetEpochSeconds()
    {
        return (int64_t)time(nullptr);
    }

    // 将time_t转换为tm
    static void TimeToTM(int64_t cur_time, tm * tm_ptr)
    {
#ifndef __GNUC__
        localtime_s(tm_ptr, &cur_time);
#else
        localtime_r((const time_t *)&cur_time, tm_ptr);
#endif
    }

	// 是否在一天(传入时间为秒)
	static bool IsInSameDay(int64_t time1, int64_t time2)
	{
		return (time1 / 60 / 60 / 24) == (time2 / 60 / 60 / 24);
	}

    // 线程睡眠
    static void ThreadSleep(int32_t milliseconds)
    {
#ifndef __GNUC__
        Sleep(milliseconds);
#else
        usleep(milliseconds * 1000);
#endif
    }
};

}

#endif