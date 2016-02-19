
#ifndef SFRAME_DELEGATE_H
#define SFRAME_DELEGATE_H

#include <assert.h>
#include <memory.h>
#include "TupleHelper.h"

namespace sframe {

// Delegate类型
enum DelegateType 
{
	kDelegateType_StaticFunctionDelegate = 1,      // 静态函数委托
	kDelegateType_MemberFunctionDelegate,          // 类成员函数委托
};

// Delegate接口
template<typename Decoder_Type>
class IDelegate
{
public:
	virtual ~IDelegate(){}
	virtual DelegateType GetType() const = 0;
	virtual bool Call(Decoder_Type& decoder) = 0;
};

// 静态函数委托
template<typename Decoder_Type, typename... Args_Type>
class StaticFunctionDelegate : public IDelegate<Decoder_Type>
{
public:
	typedef void(*FuncT)(Args_Type...);

public:
	StaticFunctionDelegate(FuncT func) : _func(func) {}
	~StaticFunctionDelegate() {}

	DelegateType GetType() const
	{
		return kDelegateType_StaticFunctionDelegate;
	}

	bool Call(Decoder_Type& decoder)
	{
		std::tuple<typename std::decay<Args_Type>::type ...> args_tuple;
		std::tuple<typename std::decay<Args_Type>::type ...> * p_args_tuple = nullptr;
		if (!decoder.Decode(&p_args_tuple, args_tuple))
		{
			return false;
		}

		if (p_args_tuple == nullptr)
		{
			p_args_tuple = &args_tuple;
		}

		UnfoldTuple(this, *p_args_tuple);
		return true;
	}

	template<typename... Args>
	void DoUnfoldTuple(Args&&... args)
	{
		this->_func(std::forward<Args>(args)...);
	}

private:
	FuncT _func;
};

// 成员函数委托接口
template<typename Decoder_Type, typename Object_Type>
class IMemberFunctionDelegate : public IDelegate<Decoder_Type>
{
public:
	virtual ~IMemberFunctionDelegate() {}

	virtual bool CallWithObject(Object_Type * obj, Decoder_Type& decoder) = 0;
};

// 成员函数委托的实现
template<typename Decoder_Type, typename Object_Type, typename... Args_Type>
class MemeberFunctionDelegate : public IMemberFunctionDelegate<Decoder_Type, Object_Type>
{
public:
	typedef void(Object_Type::*FuncT)(Args_Type...);

public:
	MemeberFunctionDelegate(FuncT func, Object_Type * obj = nullptr) : _func(func), _obj(obj), _cur_obj(nullptr){}
	~MemeberFunctionDelegate(){}

	DelegateType GetType() const
	{
		return kDelegateType_MemberFunctionDelegate;
	}

	bool Call(Decoder_Type& decoder)
	{
		assert(_obj);
		return CallWithObject(_obj, decoder);
	}

	bool CallWithObject(Object_Type * obj, Decoder_Type& decoder)
	{
		_cur_obj = obj;

		std::tuple<typename std::decay<Args_Type>::type ...> args_tuple;
		std::tuple<typename std::decay<Args_Type>::type ...> * p_args_tuple = nullptr;
		if (!decoder.Decode(&p_args_tuple, args_tuple))
		{
			return false;
		}

		if (p_args_tuple == nullptr)
		{
			p_args_tuple = &args_tuple;
		}

		UnfoldTuple(this, *p_args_tuple);
		return true;
	}

	template<typename... Args>
	void DoUnfoldTuple(Args&&... args)
	{
		(_cur_obj->*_func)(std::forward<Args>(args)...);
	}

private:
	Object_Type * _cur_obj;
	Object_Type * const _obj;
	FuncT _func;
};


// Delegate管理器
template <typename Decoder_Type, int Max_Id>
class DelegateManager
{
public:
	DelegateManager()
	{
		memset(_callers, 0, sizeof(_callers));
	}

	~DelegateManager()
	{
		for (int i = 0; i < Max_Id + 1; i++)
		{
			if (_callers[i])
			{
				delete _callers[i];
			}
		}
	}

	bool Call(int id, Decoder_Type & decoder)
	{
		if (id < 0 || id > Max_Id || !_callers[id])
		{
			return false;
		}

		return _callers[id]->Call(decoder);
	}

	template<typename Object_Type>
	bool CallWithObject(int id, Object_Type * obj, Decoder_Type & decoder)
	{
		if (id < 0 || id > Max_Id || !_callers[id] || 
			_callers[id]->GetType() != kDelegateType_MemberFunctionDelegate)
		{
			return false;
		}

		IMemberFunctionDelegate<Decoder_Type, Object_Type> * member_func_caller = dynamic_cast<IMemberFunctionDelegate<Decoder_Type, Object_Type>*>(_callers[id]);
		assert(member_func_caller);
		return member_func_caller->CallWithObject(obj, decoder);
	}

	////////// 注册函数 ////////////

	// 注册静态函数
	template<typename... Args>
	bool Regist(int id, void(*func)(Args...))
	{
		auto caller = new StaticFunctionDelegate<Decoder_Type, Args...>(func);
		assert(caller);
		return RegistCaller(id, caller);
	}

	// 注册成员函数
	template<typename Object_Type, typename... Args>
	bool Regist(int id, void(Object_Type::*func)(Args...))
	{
		auto caller = new MemeberFunctionDelegate<Decoder_Type, Object_Type, Args...>(func);
		assert(caller);
		return RegistCaller(id, caller);
	}

	// 注册成员函数同时绑定对象
	template<typename Object_Type, typename... Args>
	bool Regist(int id, void(Object_Type::*func)(Args...), Object_Type * obj)
	{
		auto caller = new MemeberFunctionDelegate<Decoder_Type, Object_Type, Args...>(func, obj);
		assert(caller);
		return RegistCaller(id, caller);
	}

private:
	bool RegistCaller(int id, IDelegate<Decoder_Type> * caller)
	{
		if (id < 0 || id > Max_Id || _callers[id])
		{
			delete caller;
			return false;
		}

		_callers[id] = caller;
		return true;
	}

private:
	IDelegate<Decoder_Type> * _callers[Max_Id + 1];
};

}

#endif