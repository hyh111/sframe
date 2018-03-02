
#ifndef SFRAME_TIMEHELPER_H
#define SFRAME_TIMEHELPER_H

#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <chrono>
#include <algorithm>

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

	static const int32_t kOneDaySeconds = 24 * 60 * 60;

	static const int32_t kOneWeekDays = 7;

	// 获取稳定的时间(从开机到现在的微妙，不能手动修改的时间)
	static int64_t GetSteadyMicroseconds()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

    // 获取稳定的时间(从开机到现在的毫秒，不能手动修改的时间)
    static int64_t GetSteadyMiliseconds()
    {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

	// 纪元到现在的微秒数（系统时间，可以手动修改的时间）
	static int64_t GetEpochMicroseconds()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

    // 纪元到现在的毫秒数（系统时间，可以手动修改的时间）
    static int64_t GetEpochMilliseconds()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 纪元到现在的秒数
    static int64_t GetEpochSeconds()
    {
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 获取本地时间
    static void LocalTime(int64_t cur_time, tm * tm_ptr)
    {
#ifndef __GNUC__
        localtime_s(tm_ptr, &cur_time);
#else
        localtime_r((const time_t *)&cur_time, tm_ptr);
#endif
    }

	// 获取UTC时间偏移秒数（UTC时间+偏移 = 本地时间）
	static int32_t GetUtcOffset()
	{
#ifndef __GNUC__
		TIME_ZONE_INFORMATION zone_info;
		GetSystemTime(&zone_info.StandardDate);
		GetTimeZoneInformation(&zone_info);
		return (-60 * zone_info.Bias);
#else
		time_t now = time(nullptr);
		struct tm cur;
		localtime_r(&now, &cur);
		return cur.tm_gmtoff;
#endif
	}

	static int32_t UtcOffset()
	{
		static int32_t utc_off = GetUtcOffset();
		return utc_off;
	}

	// 将UNIX时间戳转换为本地时间天数, 天数从0开始，即第1天为0
	// time : UNIX时间戳
	// cross_day_off_secs: 跨天偏移秒数(相对于00:00:00的秒数),取值范围：[0,86400)
	static int32_t GetLocalDay(int64_t time, int32_t cross_day_off_secs = 0)
	{
		time += UtcOffset();
		cross_day_off_secs = std::min(kOneDaySeconds - 1, std::max(0, cross_day_off_secs));
		return (int32_t)(std::max((int64_t)0, (time - cross_day_off_secs)) / kOneDaySeconds);
	}

	// 将UNIX时间戳转换为本地时间周数，周数从0开始，即第1周为0
	// time : UNIX时间戳
	// cross_week_off_day: 跨周偏移天数(相对于星期天的天数),取值范围：[0,7)
	// cross_day_off_secs: 跨天偏移秒数(相对于00:00:00的秒数),取值范围：[0,86400)
	static int32_t GetLocalWeek(int64_t time, int32_t cross_week_off_day = 0, int32_t cross_day_off_secs = 0)
	{
		int32_t day = GetLocalDay(time, cross_day_off_secs);
		cross_week_off_day = std::min(kOneWeekDays - 1, std::max(0, cross_week_off_day));
		return (std::max(0, (day + 3 - cross_week_off_day)) / kOneWeekDays);
	}

	// 是否在同一天
	// time1, time2 : UNIX时间戳
	// cross_day_off_secs: 跨天偏移秒数(相对于00:00:00的秒数),取值范围：[0,86400)
	static bool IsInSameDay(int64_t time1, int64_t time2, int32_t cross_day_off_secs = 0)
	{
		return (GetLocalDay(time1, cross_day_off_secs) == GetLocalDay(time2, cross_day_off_secs));
	}

	// 是否在同一周
	// time1, time2 : UNIX时间戳
	// cross_week_off_day: 跨周偏移天数(相对于星期天的天数),取值范围：[0,7)
	// cross_day_off_secs: 跨天偏移秒数(相对于00:00:00的秒数),取值范围：[0,86400)
	static bool IsInSameWeek(int64_t time1, int64_t time2, int32_t cross_week_off_day = 0, int32_t cross_day_off_secs = 0)
	{
		return (GetLocalWeek(time1, cross_week_off_day, cross_day_off_secs) ==
			GetLocalWeek(time2, cross_week_off_day, cross_day_off_secs));
	}

	// 获取给定的时间当天的开始的时间
	// time: 给定时间，返回的是此时间那一天的起始时间
	// cross_day_off_secs: 跨天偏移秒数
	static int64_t GetDayBeginSeconds(int64_t time, int32_t cross_day_off_secs)
	{
		cross_day_off_secs = std::min(kOneDaySeconds - 1, std::max(0, cross_day_off_secs));
		time -= cross_day_off_secs;

		struct tm t;
		LocalTime(time, &t);
		t.tm_hour = 0;
		t.tm_min = 0;
		t.tm_sec = 0;

		return (int64_t)mktime(&t) + cross_day_off_secs;
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