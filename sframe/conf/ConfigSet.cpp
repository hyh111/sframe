
#include <assert.h>
#include "ConfigSet.h"

using namespace sframe;

ConfigSet::ConfigSet(int32_t max_count) : _max_count(max_count)
{
	assert(max_count > 0);
	_config = new std::shared_ptr<void>[_max_count];
	_temp = new std::shared_ptr<void>[_max_count];
	_lock = new SpinLock[_max_count];
}

ConfigSet::~ConfigSet()
{
	delete[] _config;
	delete[] _temp;
	delete[] _lock;
}

// 加载
bool ConfigSet::Load(const std::string & dir)
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

	if (!LoadAll())
	{
		return false;
	}

	Apply();

	return true;
}

// 重新加载
bool ConfigSet::Reload()
{
	if (!LoadAll())
	{
		return false;
	}

	Apply();

	return true;
}

// 应用配置（将临时的配置替换到正式配置）
void ConfigSet::Apply()
{
	for (int i = 0; i < _max_count; i++)
	{
		if (_temp[i] != nullptr)
		{
			_lock[i].lock();
			_config[i] = _temp[i];
			_lock[i].unlock();
			_temp[i].reset();
		}
	}
}