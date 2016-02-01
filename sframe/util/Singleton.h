
#ifndef SFRAME_SINGLETON_H
#define SFRAME_SINGLETON_H

namespace sframe {

// 单例
template<typename T>
class singleton
{
private:
	struct constructor
	{
		constructor()
		{
			singleton<T>::Instance();
		}
		inline void do_nothing()const {}
	};

	static constructor s_object_constructor;

public:
	static T& Instance()
	{
		static T obj;
		s_object_constructor.do_nothing();
		return obj;
	}
};

template <typename T>
typename singleton<T>::constructor singleton<T>::s_object_constructor;

// 禁止拷贝
class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;
	noncopyable(const noncopyable & obj) = delete;
	noncopyable & operator= (const noncopyable & obj) = delete;
};

}

#endif