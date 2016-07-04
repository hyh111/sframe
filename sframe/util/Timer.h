
#ifndef SFRAME_TIMER_H
#define SFRAME_TIMER_H

#include <assert.h>
#include <inttypes.h>
#include <vector>
#include <list>
#include <memory>

namespace sframe {

// 定时器
class Timer
{
public:
	typedef uint32_t ID;

	Timer(ID id) : _timer_id(id), _exec_time(0), _del(false) {}

	virtual ~Timer() {}

	ID GetTimerID() const
	{
		return _timer_id;
	}

	void SetExecTime(int64_t exec_time)
	{
		_exec_time = exec_time;
	}

	int64_t GetExecTime() const
	{
		return _exec_time;
	}

	void SetDelete()
	{
		_del = true;
	}

	bool IsDeleted() const
	{
		return _del;
	}

	bool operator == (const Timer & t) const
	{
		return this->_timer_id == t._timer_id;
	}

	virtual int64_t Invoke() const
	{
		return 0;
	}

protected:
	ID _timer_id;
	int64_t _exec_time;  // 执行时间
	bool _del;
};

// 普通Timer(执行静态函数)
class NormalTimer : public Timer
{
public:
	// 返回下次多久后执行，小于0为停止定时器
	typedef int64_t(*TimerFunc)();

	NormalTimer(ID id, TimerFunc func) : Timer(id), _func(func) {}

	virtual ~NormalTimer() {}

	// 执行
	int64_t Invoke() const override
	{
		int64_t next = -1;
		if (_func)
		{
			next = (*_func)();
		}
		return next;
	}

private:
	TimerFunc _func;
};

// 安全定时器对象，包装一个对象，用于实现安全的Timer(外面释放对象后，不用手动删除Timer，也是安全的)
template<typename T_Obj>
class SafeTimerObj
{
public:
	SafeTimerObj() : _obj_ptr(nullptr) {}

	void SetObjectPtr(T_Obj * obj_ptr)
	{
		_obj_ptr = obj_ptr;
	}

	T_Obj * GetObjectPtr() const
	{
		return _obj_ptr;
	}

private:
	T_Obj * _obj_ptr;
};

// shared_ptr特化
template<typename T_Obj>
class SafeTimerObj<std::shared_ptr<T_Obj>>
{
public:
	SafeTimerObj() {}

	void SetObjectPtr(const std::shared_ptr<T_Obj> & obj_ptr)
	{
		_obj_ptr = obj_ptr;
	}

	T_Obj * GetObjectPtr() const
	{
		return _obj_ptr.get();
	}

private:
	std::shared_ptr<T_Obj> _obj_ptr;
};

// 定时器对象帮助
template<typename T_Obj>
struct TimerObjHelper
{
};

// 原始指针特化
template<typename T_Obj>
struct TimerObjHelper<T_Obj*>
{
	typedef T_Obj ObjectType;

	static T_Obj * GetOriginalPtr(T_Obj * obj_ptr)
	{
		return obj_ptr;
	}
};

// shared_ptr特化
template<typename T_Obj>
struct TimerObjHelper<std::shared_ptr<T_Obj>>
{
	typedef T_Obj ObjectType;

	static T_Obj * GetOriginalPtr(const std::shared_ptr<T_Obj> & obj_ptr)
	{
		return obj_ptr.get();
	}
};

// shared_ptr<SafeTimerObj>特化
template<typename T_Obj>
struct TimerObjHelper<std::shared_ptr<SafeTimerObj<T_Obj>>>
{
	typedef T_Obj ObjectType;

	static T_Obj * GetOriginalPtr(const std::shared_ptr<SafeTimerObj<T_Obj>> & obj_ptr)
	{
		return obj_ptr->GetObjectPtr();
	}
};

// SafeTimerObj特化（不允许直接使用SafeTimerObj*）
template<typename T_Obj>
struct TimerObjHelper<SafeTimerObj<T_Obj>*>
{
};

// 对象Timer(执行对象方法)
template<typename T_ObjPtr>
class ObjectTimer : public Timer
{
public:
	typedef typename TimerObjHelper<T_ObjPtr>::ObjectType TimerObjType;

	// 返回下次多久后执行，小于0为停止定时器
	typedef int64_t(TimerObjType::*TimerFunc)();

	ObjectTimer(ID id, const T_ObjPtr &  obj_ptr, TimerFunc func) : Timer(id), _obj_ptr(obj_ptr), _func(func) {}

	virtual ~ObjectTimer() {}

	// 执行
	int64_t Invoke() const override
	{
		int64_t next = -1;
		if (_func)
		{
			TimerObjType * origin_ptr = TimerObjHelper<T_ObjPtr>::GetOriginalPtr(_obj_ptr);
			if (origin_ptr)
			{
				next = (origin_ptr->*(_func))();
			}
		}
		return next;
	}

private:
	T_ObjPtr _obj_ptr;
	TimerFunc _func;
};

// 定时器管理器（仅单线程）
class TimerManager
{
public:
	// executor: 执行方法，返回下次多少毫秒后执行，小于0为停止当前的timer
	TimerManager(bool use_steady_time = true) : 
		_use_steady_time(use_steady_time), _cur_max_timer_id(0), _executing(false), _min_exec_time(0), _check_existed_when_new_id(false)
	{
		_add_temp.reserve(512);
	}

	~TimerManager();

	// 注册普通定时器
	// after_msec: 多少毫秒后执行
	// 返回大于0的id
	Timer::ID RegistNormalTimer(int64_t after_msec, NormalTimer::TimerFunc func);

	// 注册对象定时器
	template<typename T_ObjPtr>
	Timer::ID RegistObjectTimer(int64_t after_msec, typename ObjectTimer<T_ObjPtr>::TimerFunc func, const T_ObjPtr & obj_ptr)
	{
		if (!func || after_msec < 0)
		{
			assert(false);
			return 0;
		}

		ObjectTimer<T_ObjPtr> * t = new ObjectTimer<T_ObjPtr>(NewTimerId(), obj_ptr, func);
		t->SetExecTime(Now() + after_msec);
		AddTimer(t);

		return t->GetTimerID();
	}

	// 删除定时器
	void DeleteTimer(Timer::ID timer_id);

	// 执行
	void Execute();

private:
	Timer::ID NewTimerId();

	void AddTimer(Timer * t);

	int64_t Now();

private:
	Timer::ID _cur_max_timer_id;
	std::list<Timer*> _timer;
	std::vector<Timer*> _add_temp;
	int64_t _min_exec_time;
	bool _executing;
	bool _use_steady_time;
	bool _check_existed_when_new_id;   // 生成ID时是否要检查ID是否存在，当自增到最大，回归到0后变为true(一般情况下不会有到true的时候)
};


// 安全Timer注册，派生此类，用其注册定时器，对象析构后不用手动删除定时器
template<typename T>
class SafeTimerRegistor
{
public:
	SafeTimerRegistor() : _timer_mgr(nullptr)
	{
		static_assert(std::is_base_of<SafeTimerRegistor, T>::value, "T must derived from SafeTimerRegistor");
	}

	virtual ~SafeTimerRegistor() 
	{
		if (_safe_timer_obj)
		{
			_safe_timer_obj->SetObjectPtr(nullptr);
		}
	}

	void SetTimerManager(TimerManager * timer_mgr)
	{
		_timer_mgr = timer_mgr;
	}

	TimerManager * GetTimerManager() const
	{
		return _timer_mgr;
	}

	// 注册定时器(只能注册对象自身的)
	Timer::ID RegistTimer(int64_t after_msec, typename ObjectTimer<T*>::TimerFunc func)
	{
		if (_timer_mgr == nullptr)
		{
			assert(false);
			return 0;
		}

		if (!_safe_timer_obj)
		{
			_safe_timer_obj = std::make_shared<SafeTimerObj<T>>();
			_safe_timer_obj->SetObjectPtr(static_cast<T*>(this));
		}

		return _timer_mgr->RegistObjectTimer(after_msec, func, _safe_timer_obj);
	}

private:
	std::shared_ptr<SafeTimerObj<T>> _safe_timer_obj;
	TimerManager * _timer_mgr;
};

}

#endif