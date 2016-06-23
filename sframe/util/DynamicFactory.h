
#ifndef SFRAME_DYNAMIC_FACTORY_H
#define SFRAME_DYNAMIC_FACTORY_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unordered_map>
#include <string>
#include <typeinfo>

namespace sframe {

// 动态对象基类
class DynamicObject
{
public:
	DynamicObject() {}
	virtual ~DynamicObject() {}
};

// 动态对象创建工厂
class DynamicFactory
{
public:

	typedef DynamicObject* (*CreateFunction)();

	static DynamicFactory & Instance()
	{
		static DynamicFactory fac;
		return fac;
	}

	// 解析类型名称（转换为 A::B::C 的形式）
	// GCC 的type_info::name()输出的名称很猥琐，这里只做简单的解析，只支持自定义的结构体（非模板），类（非模板）、枚举、联合
	static std::string ReadTypeName(const char * name)
	{
#ifndef __GNUC__
		const char * p = strstr(name, " ");
		if (p)
		{
			size_t prev_len = (size_t)(p - name);
			if (memcmp(name, "class", prev_len) == 0 ||
				memcmp(name, "struct", prev_len) == 0 ||
				memcmp(name, "enum", prev_len) == 0 ||
				memcmp(name, "union", prev_len) == 0)
			{
				p += 1;
				return std::string(p);
			}
		}

		return std::string(name);
#else
		int cur_name_len = -1;  // 当前名字的长度（小于0表示正在读取长度）
		char cur_name_len_buf[11];
		int cur_name_len_buf_len = 0;
		char name_buf[256]; // 当前名字的buf
		int name_buf_len = 0;
		int cur_name_readed = 0;

		const char * p = name;
		while ((*p) != '\0')
		{
			char c = (*p);
			if (cur_name_len > 0)
			{
				if (cur_name_readed < cur_name_len)
				{
					if (name_buf_len > 0 && cur_name_readed == 0)
					{
						if (name_buf_len < 255)
						{
							name_buf[name_buf_len++] = ':';
						}
						if (name_buf_len < 255)
						{
							name_buf[name_buf_len++] = ':';
						}
					}

					cur_name_readed++;
					if (name_buf_len < 255)
					{
						name_buf[name_buf_len++] = c;
					}

					// 若读完了当前名称
					if (cur_name_readed >= cur_name_len)
					{
						cur_name_len = -1;
						cur_name_len_buf_len = 0;
					}
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				if (c >= '0' && c <= '9')
				{
					if (cur_name_len_buf_len < 10)
					{
						cur_name_len_buf[cur_name_len_buf_len++] = c;
					}

					// 下一个字符是否还是数字
					char next_c = *(p + 1);
					if (next_c < '0' || next_c > '9')
					{
						// 到了此处，下一个字符已经不是数字，读取长度结束
						cur_name_len_buf[cur_name_len_buf_len] = '\0';
						cur_name_len = atoi(cur_name_len_buf);
						if (cur_name_len <= 0)
						{
							cur_name_len = -1;
						}
						cur_name_len_buf_len = 0;
						cur_name_readed = 0;
					}
				}
			}

			p++;
		}

		return std::string(name_buf, name_buf_len);
#endif
	}

	bool Regist(const char * name, CreateFunction func)
	{
		if (!func)
		{
			return false;
		}
		std::string type_name = ReadTypeName(name);
		return _create_function_map.insert(std::make_pair(type_name, func)).second;
	}

	DynamicObject * Create(const std::string & type_name)
	{
		if (type_name.empty())
		{
			return nullptr;
		}

		auto it = _create_function_map.find(type_name);
		if (it == _create_function_map.end())
		{
			return nullptr;
		}

		return it->second();
	}

	template<typename T>
	T * Create(const std::string & type_name)
	{
		DynamicObject * obj = Create(type_name);
		if (!obj)
		{
			return nullptr;
		}
		T * real_obj = dynamic_cast<T*>(obj);
		if (!real_obj)
		{
			delete obj;
			return nullptr;
		}
		return real_obj;
	}

public:

	std::unordered_map<std::string, CreateFunction> _create_function_map;
};

// 动态对象创建器
template<typename T>
class DynamicCreate : public DynamicObject
{
private:
	static DynamicObject * CreateObject()
	{
		return new T();
	}

	struct Registor
	{
		Registor()
		{
			if (!DynamicFactory::Instance().Regist(typeid(T).name(), CreateObject))
			{
				assert(false);
			}
		}

		inline void do_nothing()const {}
	};

	static Registor s_registor;

public:
	virtual ~DynamicCreate()
	{
		s_registor.do_nothing();
	}
};

template <typename T>
typename DynamicCreate<T>::Registor DynamicCreate<T>::s_registor;

}

#endif