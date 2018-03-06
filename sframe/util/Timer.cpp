
#include <algorithm>
#include "Timer.h"
#include "TimeHelper.h"
#include "Log.h"

using namespace sframe;

TimerList::~TimerList()
{
	Timer * t = timer_head;
	while (t)
	{
		Timer * next = t->GetNext();
		delete t;
		t = next;
	}
}

// ×¢²áÆÕÍ¨¶¨Ê±Æ÷
// after_msec: ¶àÉÙºÁÃëºóÖ´ÐÐ
TimerHandle TimerManager::RegistNormalTimer(int32_t after_msec, NormalTimer::TimerFunc func)
{
	if (!func || after_msec < 0)
	{
		assert(false);
		return 0;
	}

	int64_t now = Now();
	if (_init_time <= 0)
	{
		assert(_exec_time <= 0);
		_init_time = now;
		_exec_time = now;
	}

	NormalTimer * t = new  NormalTimer(func);
	t->SetExecTime(now + after_msec);
	AddTimer(t);

	return t->GetHandle();
}

// É¾³ý¶¨Ê±Æ÷
void TimerManager::DeleteTimer(TimerHandle timer_handle)
{
	if (!Timer::IsTimerAlive(timer_handle))
	{
		return;
	}

	Timer * timer = timer_handle->GetTimerPtr();
	if (timer == nullptr)
	{
		assert(false);
		return;
	}

	// ²»ÄÜÉ¾³ýµ±Ç°ÕýÔÚÖ´ÐÐµÄtimer
	if (timer == _cur_exec_timer)
	{
		return;
	}

	int32_t level = -1;
	int32_t index = -1;
	timer_handle->GetLocation(&level, &index);
	int32_t tv_size;
	TimerList * tv = GetTV(level, &tv_size);
	if (tv)
	{
		if (index < 0 || index >= tv_size)
		{
			assert(false);
			return;
		}

		TimerList & timer_list = tv[index];
		timer_list.DeleteTimer(timer);
		delete timer;
	}
	else
	{
		auto it = std::find(_add_timer_cache.begin(), _add_timer_cache.end(), timer);
		if (it == _add_timer_cache.end())
		{
			// Ã»ÓÐÔÚÖ´ÐÐ×éÖÐ£¬¿Ï¶¨ÔÚcacheÖÐ£¬Ã»ÓÐÕÒµ½ËµÃ÷Âß¼­ÓÐ´íÎó
			assert(false);
			return;
		}

		_add_timer_cache.erase(it);
		delete timer;
	}
}

// Ö´ÐÐ
void TimerManager::Execute()
{
	if (_init_time <= 0 || _exec_time <= 0)
	{
		return;
	}

	int64_t now = Now();
	if (now < _exec_time)
	{
		return;
	}

	do
	{
		int64_t init_to_exec_tick = (_exec_time - _init_time) / kMilliSecOneTick;
		int64_t index = init_to_exec_tick & TVR_MASK;
		if (index <= 0 && _exec_time != _init_time &&
			Cascade(_tv2, (int32_t)((init_to_exec_tick >> TVR_BITS) & TVN_MASK)) <= 0 &&
			Cascade(_tv3, (int32_t)((init_to_exec_tick >> (TVR_BITS + TVN_BITS)) & TVN_MASK)) <= 0 &&
			Cascade(_tv4, (int32_t)((init_to_exec_tick >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK)) <= 0)
		{
			Cascade(_tv5, (int32_t)((init_to_exec_tick >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK));
		}

		TimerList & cur_list = _tv1[index];
		_cur_exec_timer = cur_list.timer_head;
		while (_cur_exec_timer)
		{
			// Ö´ÐÐ
			int64_t after = -1;
			try
			{
				after = _cur_exec_timer->Invoke();
			}
			catch(const std::exception& e)
			{
				LOG_ERROR << "Execute timer|std::exception|" << e.what() << std::endl;
				after = -1;
			}
			catch (...)
			{
				LOG_ERROR << "Execute timer|std::exception" << std::endl;
				after = -1;
			}

			// »ñÈ¡ÏÂÒ»½Úµã£¬²¢É¾³ýµ±Ç°½Úµã
			Timer * next_timer = _cur_exec_timer->GetNext();
			cur_list.DeleteTimer(_cur_exec_timer);

			if (after >= 0)
			{
				_cur_exec_timer->SetExecTime(now + after);
				_add_timer_cache.push_back(_cur_exec_timer);
			}
			else
			{
				delete _cur_exec_timer;
			}

			_cur_exec_timer = next_timer;
		}

		if (now - _exec_time < kMilliSecOneTick)
		{
			if (!_add_timer_cache.empty())
			{
				for (Timer * t : _add_timer_cache)
				{
					AddTimer(t);
				}
				_add_timer_cache.clear();
			}
		}

		_exec_time += kMilliSecOneTick;

	} while (now >= _exec_time);
}

int32_t TimerManager::Cascade(TimerList * tv, int32_t index)
{
	TimerList & timer_list = tv[index];

	Timer * cur = timer_list.timer_head;
	timer_list.timer_head = nullptr;
	timer_list.timer_tail = nullptr;

	while (cur)
	{
		Timer * next = cur->GetNext();
		cur->SetNext(nullptr);
		cur->SetPrev(nullptr);
		AddTimer(cur);

		cur = next;
	}

	return index;
}

void TimerManager::AddTimer(Timer * t)
{
	if (_cur_exec_timer)
	{
		_add_timer_cache.push_back(t);
		return;
	}

	int64_t timer_time = t->GetExecTime();
	if (_exec_time <= 0 || _init_time <= 0 || timer_time < _exec_time)
	{
		assert(false);
		return;
	}

	int32_t after_tick = (int32_t)MilliSecToTick(timer_time - _exec_time, true);
	timer_time = _exec_time + after_tick * kMilliSecOneTick;
	t->SetExecTime(timer_time);
	int64_t init_to_exec_tick = (timer_time - _init_time) / kMilliSecOneTick;

	if (after_tick < (1 << TVR_BITS))
	{
		int64_t index = init_to_exec_tick & TVR_MASK;
		_tv1[index].AddTimer(t);
		t->GetHandle()->SetLocation(1, (int32_t)index);
	}
	else if (after_tick < (1 << (TVR_BITS + TVN_BITS)))
	{
		int64_t index = (init_to_exec_tick >> TVR_BITS) & TVN_MASK;
		_tv2[index].AddTimer(t);
		t->GetHandle()->SetLocation(2, (int32_t)index);
	}
	else if (after_tick < (1 << (TVR_BITS + 2 * TVN_BITS)))
	{
		int64_t index = (init_to_exec_tick >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		_tv3[index].AddTimer(t);
		t->GetHandle()->SetLocation(3, (int32_t)index);
	}
	else if (after_tick < (1 << (TVR_BITS + 3 * TVN_BITS)))
	{
		int64_t index = (init_to_exec_tick >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		_tv4[index].AddTimer(t);
		t->GetHandle()->SetLocation(4, (int32_t)index);
	}
	else
	{
		int64_t index = (init_to_exec_tick >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		_tv5[index].AddTimer(t);
		t->GetHandle()->SetLocation(5, (int32_t)index);
	}
}

TimerList * TimerManager::GetTV(int32_t level, int32_t * size)
{
	TimerList * tv = nullptr;
	switch (level)
	{
	case 1:
		tv = _tv1;
		(*size) = TVR_SIZE;
		break;
	case 2:
		tv = _tv2;
		(*size) = TVN_SIZE;
		break;
	case 3:
		tv = _tv3;
		(*size) = TVN_SIZE;
		break;
	case 4:
		tv = _tv4;
		(*size) = TVN_SIZE;
		break;
	case 5:
		tv = _tv5;
		(*size) = TVN_SIZE;
		break;
	}
	return tv;
}

int64_t TimerManager::MilliSecToTick(int64_t millisec, bool ceil)
{
	if (ceil)
	{
		return (millisec % kMilliSecOneTick > 0) ? (millisec / kMilliSecOneTick + 1) : (millisec / kMilliSecOneTick);
	}

	return millisec / kMilliSecOneTick;
}

int64_t TimerManager::Now()
{
	return TimeHelper::GetSteadyMiliseconds();
}
