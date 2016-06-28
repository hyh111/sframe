
#include <memory.h>
#include <assert.h>
#include "ConfigSet.h"

using namespace sframe;

ConfigSet::ConfigSet(int32_t max_count) : _max_count(max_count)
{
	assert(max_count > 0);
	_config = new std::shared_ptr<ConfigBase>[_max_count];
	_load_config_function = new LoadConfigFunction[_max_count];
	memset(_load_config_function, 0, sizeof(LoadConfigFunction) * _max_count);
	_get_config_name_function = new GetConfigNameFunction[_max_count];
	memset(_get_config_name_function, 0, sizeof(GetConfigNameFunction) * _max_count);
	_init_config_function = new InitConfigFunction[_max_count];
	memset(_init_config_function, 0, sizeof(InitConfigFunction) * _max_count);
	_lock = new Lock[_max_count];
}

ConfigSet::~ConfigSet()
{
	delete[] _config;
	delete[] _load_config_function;
	delete[] _get_config_name_function;
	delete[] _init_config_function;
	delete[] _lock;
}

// 加载
bool ConfigSet::Initialize(const std::string & dir, std::string * log_msg)
{
	_config_dir = dir;
	if (!_config_dir.empty())
	{
		for (auto it = _config_dir.begin(); it < _config_dir.end(); it++)
		{
			if (*it == '\\')
			{
				*it = '/';
			}
		}

		if (*(_config_dir.end() - 1) != '/')
		{
			_config_dir.push_back('/');
		}
	}

	RegistAllConfig();

	return Reload(log_msg);
}

// 重新加载
bool ConfigSet::Reload(std::string * log_msg)
{
	assert(!_config_dir.empty());

	std::shared_ptr<ConfigBase> * config_temp = new std::shared_ptr<ConfigBase>[_max_count];

	bool ret = true;

	// 加载
	for (int i = 0; i < _max_count; i++)
	{
		if (!_load_config_function[i])
		{
			continue;
		}

		assert(_get_config_name_function[i]);

		config_temp[i] = (this->*_load_config_function[i])();
		if (!config_temp[i])
		{
			ret = false;
			if (log_msg)
			{
				log_msg->append("load config " + std::string((*_get_config_name_function[i])()) + " error\n");
			}
		}
	}

	// 初始化
	for (int i = 0; i < _max_count; i++)
	{
		if (!config_temp[i])
		{
			continue;
		}

		assert(_init_config_function[i]);
		if (!(*_init_config_function[i])(config_temp[i]))
		{
			if (log_msg)
			{
				log_msg->append("init config " + std::string((*_get_config_name_function[i])()) + " error\n");
			}
		}
		else
		{
			AUTO_LOCK(_lock[i]);
			_config[i] = config_temp[i];
		}
	}

	delete[] config_temp;
	return ret;
}
