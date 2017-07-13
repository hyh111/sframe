
#ifndef SFRAME_TIMER_H
#define SFRAME_TIMER_H

#include <assert.h>
#include <inttypes.h>
#include <vector>
#include <memory>

namespace sframe {

class Timer;

class TimerWrapper
{
	friend class Timer;
	friend class TimerManager;
	friend struct TimerList;
public:
	TimerWrapper(Timer * timer) : _timer(timer), _level(-1), _index(-1) {}
	~TimerWrapper() {}

private:
	void SetDeleted() { _timer = nullptr; }

	void SetLocation(int32_t level, int32_t index) 
	{
		_level = level;
		_index = index;
	}

	Timer * GetTimerPtr() { return _timer; }

	void GetLocation(int32_t * level, int32_t * index) const
	{
		(*level) = _level;
		(*index) = _index;
	}

	Timer * _timer;
	int32_t _level;
	int32_t _index;
};

typedef std::shared_ptr<TimerWrapper> TimerHandle;

// 定时器
class Timer
{
public:

	static bool IsTimerAlive(const TimerHandle & timer_handle)
	{
		return timer_handle ? timer_handle->_timer != nullptr : false;
	}

	Timer() : _exec_time(0), _prev(nullptr), _next(nullptr)
	{
		_handle = std::make_shared<TimerWrapper>(this);
	}

	virtual ~Timer()
	{
		_handle->SetDeleted();
	}

	const TimerHandle & GetHandle() const
	{
		return _handle;
	}

	void SetExecTime(int64_t exec_time)
	{
		_exec_time = exec_time;
	}

	int64_t GetExecTime() const
	{
		return _exec_time;
	}

	Timer * GetPrev() const
	{
		return _prev;
	}

	Timer * GetNext() const
	{
		return _next;
	}

	void SetPrev(Timer * t)
	{
		_prev = t;
	}

	void SetNext(Timer * t)
	{
		_next = t;
	}

	virtual int32_t Invoke() const = 0;

protected:
	TimerHandle _handle;
	int64_t _exec_time;     // 执行时间
	Timer * _prev;
	Timer * _next;
};

// 普通Timer(执行静态函数)
class NormalTimer : public Timer
{
public:
	// 返回下次多久后执行，小于0为停止定时器
	typedef int32_t(*TimerFunc)();

	NormalTimer(TimerFunc func) : _func(func) {}

	virtual ~NormalTimer() {}

	// 执行
	int32_t Invoke() const override
	{
		int32_t next = -1;
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
	typedef int32_t(TimerObjType::*TimerFunc)();

	ObjectTimer(const T_ObjPtr &  obj_ptr, TimerFunc func) : _obj_ptr(obj_ptr), _func(func) {}

	virtual ~ObjectTimer() {}

	// 执行
	int32_t Invoke() const override
	{
		int32_t next = -1;
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

// 定时器链表
struct TimerList
{
	TimerList() : timer_head(nullptr), timer_tail(nullptr) {}

	~TimerList();

	void DeleteTimer(Timer * t)
	{
		if (t == nullptr)
		{
			return;
		}

		Timer * prev = t->GetPrev();
		Timer * next = t->GetNext();
		if (prev && next)
		{
			assert(t != timer_head && t != timer_tail);
			prev->SetNext(next);
			next->SetPrev(prev);
		}
		else if (prev)
		{
			assert(t == timer_tail);
			timer_tail = prev;
			timer_tail->SetNext(nullptr);
		}
		else if (next)
		{
			assert(t == timer_head);
			timer_head = next;
			timer_head->SetPrev(nullptr);
		}
		else
		{
			assert(t == timer_head && t == timer_tail);
			timer_head = nullptr;
			timer_tail = nullptr;
		}

		t->SetPrev(nullptr);
		t->SetNext(nullptr);
		t->GetHandle()->SetLocation(-1, -1);
	}

	void AddTimer(Timer * t)
	{
		assert(t->GetPrev() == nullptr && t->GetNext() == nullptr);
		if (timer_tail == nullptr)
		{
			assert(timer_head == nullptr);
			timer_head = t;
			timer_tail = t;
		}
		else
		{
			t->SetPrev(timer_tail);
			timer_tail->SetNext(t);
			timer_tail = t;
		}
	}

	bool IsEmpty() const
	{
		return timer_head == nullptr;
	}

	Timer* timer_head;           // 定时器链表头
	Timer* timer_tail;           // 定时器链表头
};

#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK ((int64_t)(TVN_SIZE - 1))
#define TVR_MASK ((int64_t)(TVR_SIZE - 1))

struct tvec {
	struct TimerList vec[TVN_SIZE];
};

struct tvec_root {
	struct TimerList vec[TVR_SIZE];
};

// 定时器管理器
class TimerManager
{
public:
	static const int32_t kMilliSecOneTick = 1;                  // 一个tick多少毫秒

	TimerManager() : _exec_time(0), _cur_exec_timer(nullptr)
	{
		_add_timer_cache.reserve(128);
	}

	~TimerManager() {}

	// 注册普通定时器
	// after_msec: 多少毫秒后执行
	TimerHandle RegistNormalTimer(int32_t after_msec, NormalTimer::TimerFunc func);

	// 注册对象定时器
	template<typename T_ObjPtr>
	TimerHandle RegistObjectTimer(int32_t after_msec, typename ObjectTimer<T_ObjPtr>::TimerFunc func, const T_ObjPtr & obj_ptr)
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

		ObjectTimer<T_ObjPtr> * t = new ObjectTimer<T_ObjPtr>(obj_ptr, func);
		t->SetExecTime(now + after_msec);
		AddTimer(t);

		return t->GetHandle();
	}

	// 删除定时器
	void DeleteTimer(TimerHandle timer_handle);

	// 执行
	void Execute();

private:
	int32_t Cascade(TimerList * tv, int32_t index);

	void AddTimer(Timer * t);

	TimerList * GetTV(int32_t level, int32_t * size);

	static int64_t MilliSecToTick(int64_t millisec, bool ceil);

	int64_t Now();

private:
	TimerList _tv1[TVR_SIZE];
	TimerList _tv2[TVN_SIZE];
	TimerList _tv3[TVN_SIZE];
	TimerList _tv4[TVN_SIZE];
	TimerList _tv5[TVN_SIZE];
	int64_t _exec_time;
	int64_t _init_time;
	std::vector<Timer*> _add_timer_cache;                // 添加定时器缓存
	Timer * _cur_exec_timer;
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
	TimerHandle RegistTimer(int32_t after_msec, typename ObjectTimer<T*>::TimerFunc func)
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