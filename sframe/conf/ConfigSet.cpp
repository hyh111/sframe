
#include <memory.h>
#include <assert.h>
#include <sstream>
#include "ConfigSet.h"

using namespace sframe;


struct LoadItem
{
	int32_t conf_id;
	ConfigSet::ConfUnit * conf_unit;
	ConfigSet::ConfigLoadHelper * load_helper;
};



ConfigSet::ConfigSet()
{
	memset(&_quick_find_arr, 0, sizeof(_quick_find_arr));
}

ConfigSet::~ConfigSet()
{
	for (auto & pr : _config)
	{
		delete pr.second;
	}
}

// ¼ÓÔØ(È«²¿³É¹¦·µ»Øtrue, Ö»ÒªÓÐÒ»¸öÊ§°Ü¶¼»á·µ»Øfalse)
// ³ö´íÊ±£¬err_info»á·µ»Ø³ö´íµÄÅäÖÃÐÅÏ¢
bool ConfigSet::Load(const std::string & path, std::vector<std::string> * vec_err_msg)
{
	if (path.empty() || !_config.empty())
	{
		return false;
	}

	_config_dir = path;
	std::vector<LoadItem> vec_load_items;
	vec_load_items.reserve(_config_load_helper.size() + _temporary_config_load_helper.size());

	bool ret = true;

	// ¼ÓÔØÅäÖÃ
	for (auto & pr : _config_load_helper)
	{
		LoadItem item;
		item.conf_id = pr.first;
		item.load_helper = &pr.second;

		// ¼ÓÔØ
		std::vector<std::string> vec_err_files;
		item.conf_unit = (this->*item.load_helper->func_load)(item.load_helper->conf_file_name, &vec_err_files);
		if (item.conf_unit == nullptr)
		{
			ret = false;
			if (vec_err_msg)
			{
				std::ostringstream oss;
				for (const std::string & cur_file_name : vec_err_files)
				{
					oss << "Load config error, file(" << cur_file_name << ")" << std::endl;
				}
				vec_err_msg->push_back(oss.str());
			}
			continue;
		}

		vec_load_items.push_back(item);

		// ±£´æÏÂÀ´
		if (_config.insert(std::make_pair(item.conf_id, item.conf_unit)).second)
		{
			if (item.conf_id >= 0 && item.conf_id < kQuickFindArrLen)
			{
				_quick_find_arr[item.conf_id] = item.conf_unit;
			}
		}
		else
		{
			assert(false);
		}
	}

	// ¼ÓÔØÁÙÊ±ÅäÖÃ
	for (auto & load_helper : _temporary_config_load_helper)
	{
		LoadItem item;
		item.conf_id = load_helper.conf_id;
		item.load_helper = &load_helper;

		// ¼ÓÔØ
		std::vector<std::string> vec_err_files;
		item.conf_unit = (this->*item.load_helper->func_load)(item.load_helper->conf_file_name, &vec_err_files);
		if (item.conf_unit == nullptr)
		{
			ret = false;
			if (vec_err_msg)
			{
				std::ostringstream oss;
				for (const std::string & cur_file_name : vec_err_files)
				{
					oss << "Load config error, file(" << cur_file_name << ")" << std::endl;
				}
				vec_err_msg->push_back(oss.str());
			}
			continue;
		}

		vec_load_items.push_back(item);
	}

	if (!ret)
	{
		return false;
	}

	// ³õÊ¼»¯ËùÓÐÅäÖÃ
	for (auto & load_item : vec_load_items)
	{
		if (!(this->*load_item.load_helper->func_init)(load_item.conf_unit))
		{
			ret = false;
			if (vec_err_msg)
			{
				std::ostringstream oss;
				oss << "Init config error, conf(" << load_item.load_helper->conf_type_name << ")" << std::endl;
				vec_err_msg->push_back(oss.str());
			}
		}
	}

	return ret;
}
