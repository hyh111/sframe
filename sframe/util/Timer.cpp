
#include <algorithm>
#include "Timer.h"
#include "TimeHelper.h"

using namespace sframe;

TimerManager::~TimerManager()
{
	assert(_add_temp.empty());
	for (Timer * t : _timer)
	{
		if (t)
		{
			delete t;
		}
	}
}

// 注册普通定时器
// after_msec: 多少毫秒后执行
// 返回大于0的id
Timer::ID TimerManager::RegistNormalTimer(int64_t after_msec, NormalTimer::TimerFunc func)
{
	if (!func || after_msec < 0)
	{
		assert(false);
		return 0;
	}

	NormalTimer * t = new  NormalTimer(NewTimerId(), func);
	t->SetExecTime(Now() + after_msec);
	AddTimer(t);

	return t->GetTimerID();
}

class TimerFindor
{
public:
	TimerFindor(Timer::ID timer_id) : _timer_id(timer_id) {}

	bool operator()(const Timer * timer)
	{
		return timer->GetTimerID() == _timer_id;
	}

private:
	Timer::ID _timer_id;
};

// 删除定时器
void TimerManager::DeleteTimer(Timer::ID timer_id)
{
	TimerFindor findor(timer_id);

	if (_executing)
	{
		auto it_timer = std::find_if(_timer.begin(), _timer.end(), findor);
		if (it_timer != _timer.end())
		{
			(*it_timer)->SetDelete();
		}
		else
		{
			auto it_tmp = std::find_if(_add_temp.begin(), _add_temp.end(), findor);
			if (it_tmp != _add_temp.end())
			{
				delete (*it_tmp);
				_add_temp.erase(it_tmp);
			}
		}
	}
	else
	{
		auto it = std::find_if(_timer.begin(), _timer.end(), findor);
		if (it != _timer.end())
		{
			delete (*it);
			_timer.erase(it);
		}
	}
}

// 执行
void TimerManager::Execute()
{
	int64_t cur_time = Now();
	if (_min_exec_time <= 0 || cur_time < _min_exec_time)
	{
		return;
	}

	_min_exec_time = 0;
	_executing = true;
	assert(_add_temp.empty());
	auto it = _timer.begin();

	while (it != _timer.end())
	{
		Timer * cur = *it;
		if (cur->IsDeleted())
		{
			delete cur;
			it = _timer.erase(it);
			continue;
		}

		if (cur_time >= cur->GetExecTime())
		{
			int64_t after_ms = cur->Invoke();
			if (after_ms < 0)
			{
				delete cur;
				it = _timer.erase(it);
				continue;
			}
			else
			{
				cur->SetExecTime(cur_time + after_ms);
				if (_min_exec_time <= 0 || cur->GetExecTime() < _min_exec_time)
				{
					_min_exec_time = cur->GetExecTime();
				}
			}
		}

		it++;
	}

	_executing = false;

	// 将add_temp中的timer加入timer列表
	for (Timer * t : _add_temp)
	{
		assert(t);
		_timer.push_back(t);
		if (_min_exec_time <= 0 || t->GetExecTime() < _min_exec_time)
		{
			_min_exec_time = t->GetExecTime();
		}
	}
	_add_temp.clear();
}

Timer::ID TimerManager::NewTimerId()
{
	if (_cur_max_timer_id == 0xffffffff)
	{
		_cur_max_timer_id = 0;
	}

	return ++_cur_max_timer_id;
}

void TimerManager::AddTimer(Timer * t)
{
	if (_executing)
	{
		_add_temp.push_back(t);
	}
	else
	{
		_timer.push_back(t);
		if (_min_exec_time <= 0 || t->GetExecTime() < _min_exec_time)
		{
			_min_exec_time = t->GetExecTime();
		}
	}
}

int64_t TimerManager::Now()
{
	return (_use_steady_time ? TimeHelper::GetSteadyMiliseconds() : TimeHelper::GetEpochMilliseconds());
}