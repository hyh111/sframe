
#ifndef SFRAME_DELEGATE_H
#define SFRAME_DELEGATE_H

#include <assert.h>
#include <memory.h>
#include <unordered_map>
#include <functional>
#include "TupleHelper.h"

namespace sframe {

// DelegateÀàÐÍ
enum DelegateType 
{
	kDelegateType_None = 0,
	kDelegateType_StaticFuncDelegate,                              // ¾²Ì¬º¯ÊýÎ¯ÍÐ
	kDelegateType_MemberFuncDelegate_WithObject,                   // ³ÉÔ±º¯ÊýÎ¯ÍÐ(Í¨¹ý¶ÔÏóµ÷ÓÃ)
	kDelegateType_MemberFuncDelegate_WithObjectFinder,             // ³ÉÔ±º¯ÊýÎ¯ÍÐ(Í¨¹ý¶ÔÏó²éÕÒÆ÷µ÷ÓÃ)
};

// Delegate½Ó¿Ú
template<typename Decoder_Type>
class IDelegate
{
public:
	virtual ~IDelegate(){}
	virtual DelegateType GetType() const = 0;
	virtual bool Call(Decoder_Type& decoder) = 0;
};

// ¾²Ì¬º¯ÊýÎ¯ÍÐ
template<typename Decoder_Type, typename... Args_Type>
class StaticFuncDelegate : public IDelegate<Decoder_Type>
{
public:
	typedef void(*FuncT)(Args_Type...);

public:
	StaticFuncDelegate(FuncT func) : _func(func) {}
	~StaticFuncDelegate() {}

	DelegateType GetType() const
	{
		return kDelegateType_StaticFuncDelegate;
	}

	bool Call(Decoder_Type& decoder) override
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

// ³ÉÔ±º¯ÊýÎ¯ÍÐ½Ó¿Ú(Í¨¹ý¶ÔÏóµ÷ÓÃ)
template<typename Decoder_Type, typename Object_Type>
class IMemberFuncDelegate_WithObject : public IDelegate<Decoder_Type>
{
public:
	virtual ~IMemberFuncDelegate_WithObject() {}

	virtual bool CallWithObject(Object_Type * obj, Decoder_Type& decoder) = 0;
};

// ³ÉÔ±º¯ÊýÎ¯ÍÐµÄÊµÏÖ
template<typename Decoder_Type, typename Object_Type, typename... Args_Type>
class MemeberFunctionDelegate_WithObject : public IMemberFuncDelegate_WithObject<Decoder_Type, Object_Type>
{
public:
	typedef void(Object_Type::*FuncT)(Args_Type...);

	MemeberFunctionDelegate_WithObject(FuncT func, Object_Type * obj = nullptr) : _cur_obj(nullptr), _obj(obj), _func(func) {}

	~MemeberFunctionDelegate_WithObject(){}

	DelegateType GetType() const override
	{
		return kDelegateType_MemberFuncDelegate_WithObject;
	}

	bool Call(Decoder_Type& decoder) override
	{
		if (!_obj)
		{
			assert(false);
			return false;
		}
		return CallWithObject(_obj, decoder);
	}

	bool CallWithObject(Object_Type * obj, Decoder_Type& decoder) override
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

// ³ÉÔ±º¯ÊýÎ¯ÍÐ½Ó¿Ú(Í¨¹ý¶ÔÏó²éÕÒÆ÷µ÷ÓÃ)
template<typename Decoder_Type, typename Object_Key_Type>
class IMemberFuncDelegate_WithObjectFinder : public IDelegate<Decoder_Type>
{
public:
	virtual ~IMemberFuncDelegate_WithObjectFinder() {}

	bool Call(Decoder_Type& decoder) override
	{
		return false;
	}

	virtual bool CallWithObjectKey(const Object_Key_Type & obj_key, Decoder_Type& decoder) = 0;
};

// ³ÉÔ±º¯ÊýÎ¯ÍÐ(Í¨¹ý¶ÔÏó²éÕÒÆ÷µ÷ÓÃ)
template<typename Decoder_Type, typename Object_Key_Type, typename Object_Type, typename... Args_Type>
class MemberFuncDelegate_WithObjectFinder : public IMemberFuncDelegate_WithObjectFinder<Decoder_Type, Object_Key_Type>
{
public:
	typedef void(Object_Type::*FuncT)(Args_Type...);

	typedef std::function<Object_Type*(const Object_Key_Type & obj_key)> ObjectFinder;

	MemberFuncDelegate_WithObjectFinder(FuncT func, const ObjectFinder & obj_finder) : _cur_obj(nullptr), _func(func), _obj_finder(obj_finder) {}

	~MemberFuncDelegate_WithObjectFinder() {}

	DelegateType GetType() const override
	{
		return kDelegateType_MemberFuncDelegate_WithObjectFinder;
	}

	bool CallWithObjectKey(const Object_Key_Type & obj_key, Decoder_Type& decoder) override
	{
		Object_Type * obj = nullptr;
		if (_obj_finder)
		{
			obj = _obj_finder(obj_key);
		}

		if (!obj)
		{
			return false;
		}

		return CallWithObject(obj, decoder);
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
	FuncT _func;
	ObjectFinder _obj_finder;
};


// Delegate¹ÜÀíÆ÷
template <typename Decoder_Type>
class DelegateManager
{
	static const int kMaxArrLen = 65536;

	typedef int64_t DefaultObjectKeyType;

public:

	DelegateManager()
	{
		memset(_callers, 0, sizeof(_callers));
	}

	~DelegateManager()
	{
		for (int i = 0; i < kMaxArrLen; i++)
		{
			if (_callers[i])
			{
				delete _callers[i];
			}
		}

		for (auto it = _map_callers.begin(); it != _map_callers.end(); it++)
		{
			if (it->second)
			{
				delete it->second;
			}
		}
	}

	DelegateType GetMsgDelegateType(int32_t msg_id)
	{
		auto caller = GetCaller(msg_id);
		if (!caller)
		{
			return kDelegateType_None;
		}

		return caller->GetType();
	}

	bool Call(int id, Decoder_Type & decoder)
	{
		auto caller = GetCaller(id);
		if (!caller)
		{
			return false;
		}

		return caller->Call(decoder);
	}

	template<typename Object_Type>
	bool CallWithObject(int id, Object_Type * obj, Decoder_Type & decoder)
	{
		auto caller = GetCaller(id);
		if (!caller || caller->GetType() != kDelegateType_MemberFuncDelegate_WithObject)
		{
			return false;
		}

		IMemberFuncDelegate_WithObject<Decoder_Type, Object_Type> * member_func_caller =
			dynamic_cast<IMemberFuncDelegate_WithObject<Decoder_Type, Object_Type>*>(_callers[id]);

		if (!member_func_caller)
		{
			assert(false);
			return false;
		}

		return member_func_caller->CallWithObject(obj, decoder);
	}

	template<typename Object_Key_Type>
	bool CallWithObjectKey(int id, const Object_Key_Type & obj_key, Decoder_Type & decoder)
	{
		auto caller = GetCaller(id);
		if (!caller || caller->GetType() != kDelegateType_MemberFuncDelegate_WithObjectFinder)
		{
			return false;
		}

		IMemberFuncDelegate_WithObjectFinder<Decoder_Type, Object_Key_Type> * member_func_caller =
			dynamic_cast<IMemberFuncDelegate_WithObjectFinder<Decoder_Type, Object_Key_Type>*>(_callers[id]);

		if (!member_func_caller)
		{
			assert(false);
			return false;
		}

		return member_func_caller->CallWithObjectKey(obj_key, decoder);
	}

	////////// ×¢²áº¯Êý ////////////

	// ×¢²á¾²Ì¬º¯Êý
	template<typename... Args>
	DelegateType Regist(int id, void(*func)(Args...))
	{
		auto caller = new StaticFuncDelegate<Decoder_Type, Args...>(func);
		assert(caller);
		if (!RegistCaller(id, caller))
		{
			assert(false);
			return kDelegateType_None;
		}
		return caller->GetType();
	}

	// ×¢²á³ÉÔ±º¯Êý
	template<typename Object_Type, typename... Args>
	DelegateType Regist(int id, void(Object_Type::*func)(Args...))
	{
		auto caller = new MemeberFunctionDelegate_WithObject<Decoder_Type, Object_Type, Args...>(func);
		assert(caller);
		if (!RegistCaller(id, caller))
		{
			assert(false);
			return kDelegateType_None;
		}
		return caller->GetType();
	}

	// ×¢²á³ÉÔ±º¯ÊýÍ¬Ê±°ó¶¨¶ÔÏó
	template<typename Object_Type, typename... Args>
	DelegateType Regist(int id, void(Object_Type::*func)(Args...), Object_Type * obj)
	{
		if (!obj)
		{
			assert(false);
			return kDelegateType_None;
		}

		auto caller = new MemeberFunctionDelegate_WithObject<Decoder_Type, Object_Type, Args...>(func, obj);
		assert(caller);
		if (!RegistCaller(id, caller))
		{
			assert(false);
			return kDelegateType_None;
		}
		return caller->GetType();
	}

	// ×¢²á³ÉÔ±º¯ÊýÍ¬Ê±°ó¶¨¶ÔÏó²éÕÒÆ÷
	template<typename Object_Key_Type, typename Object_Type, typename... Args>
	DelegateType Regist(int id, void(Object_Type::*func)(Args...), const std::function<Object_Type*(const Object_Key_Type & obj_key)> & obj_finder)
	{
		if (!obj_finder)
		{
			assert(false);
			return kDelegateType_None;
		}

		auto caller = new MemberFuncDelegate_WithObjectFinder<Decoder_Type, Object_Key_Type, Object_Type, Args...>(func, obj_finder);
		assert(caller);
		if (!RegistCaller(id, caller))
		{
			assert(false);
			return kDelegateType_None;
		}
		return caller->GetType();
	}

private:
	bool RegistCaller(int id, IDelegate<Decoder_Type> * caller)
	{
		if (GetCaller(id) != nullptr)
		{
			delete caller;
			return false;
		}

		if (id >= 0 || id < kMaxArrLen)
		{
			_callers[id] = caller;
		}
		else
		{
			_map_callers[id] = caller;
		}

		return true;
	}

	IDelegate<Decoder_Type> * GetCaller(int id)
	{
		if (id >= 0 && id < kMaxArrLen)
		{
			return _callers[id];
		}

		auto it = _map_callers.find(id);
		if (it != _map_callers.end())
		{
			return it->second;
		}

		return nullptr;
	}

private:
	IDelegate<Decoder_Type> * _callers[kMaxArrLen];
	std::unordered_map<int32_t, IDelegate<Decoder_Type> *> _map_callers;
};

}

#endif