
#include <assert.h>
#include <algorithm>
#include <sstream>
#include <memory>
#include "ServerConfig.h"
#include "conf/JsonReader.h"
#include "util/FileHelper.h"
#include "util/Log.h"
#include "util/Convert.h"

using namespace sframe;

bool NetAddrInfo::ParseFormString(const std::string & data)
{
	if (data.empty())
	{
		return false;
	}

	std::size_t pos = data.find(":");
	if (pos == std::string::npos || pos == 0 || pos >= data.length() - 1)
	{
		return false;
	}

	ip = data.substr(0, pos);
	port = sframe::StrToAny<uint16_t>(data.substr(pos + 1));
	if (port == 0)
	{
		return false;
	}

	return true;
}

bool ListenAddrInfo::ParseFormString(const std::string & data)
{
	if (data.empty())
	{
		return false;
	}

	std::string addr_str;
	std::size_t pos = data.find("@");
	if (pos != std::string::npos)
	{
		desc = data.substr(0, pos);
		addr_str = data.substr(pos + 1);
	}
	else
	{
		addr_str = data;
	}

	return addr.ParseFormString(addr_str);
}

bool ServiceInfo::ParseFormString(const std::string & data)
{
	if (data.empty())
	{
		return false;
	}

	std::size_t pos = data.find("@");
	if (pos == std::string::npos)
	{
		service_type_name = data;
		is_local_service = true;
	}
	else
	{
		if (pos == 0 || pos >= data.length() - 1)
		{
			return false;
		}

		service_type_name = data.substr(0, pos);
		is_local_service = false;
		if (!remote_addr.ParseFormString(data.substr(pos + 1)))
		{
			return false;
		}
	}

	return true;
}

// 去注释
std::string RemoveComments(const std::string & data)
{
	int cur_state = 0;   // 0 不是注释, 1 行注释, 2段注释 
	std::ostringstream oss;
	auto it = data.begin();
	while (it < data.end())
	{
		char c = *it;
		if (cur_state == 1)
		{
			if (c == '\n')
			{
				// 行注释结束
				cur_state = 0;
			}
		}
		else if (cur_state == 2)
		{
			if (c == '*' &&  it < data.end() - 1 && (*(it + 1)) == '/')
			{
				it++;
				// 段注释结束
				cur_state = 0;
			}
		}
		else
		{
			if (c == '/' && it < data.end() - 1)
			{
				char next = *(it + 1);
				if (next == '/')
				{
					cur_state = 1;
					it += 2;
					continue;
				}
				else if (next == '*')
				{
					cur_state = 2;
					it += 2;
					continue;
				}
			}

			oss << c;
		}

		it++;
	}

	return oss.str();
}

bool ServerConfig::Load(const std::string & filename)
{
	std::string content;
	if (!FileHelper::ReadFile(filename, content))
	{
		LOG_ERROR << "Open config file(" << filename << ") error" << ENDL;
		return false;
	}

	std::string err;
	std::string real_content = RemoveComments(content);
	json11::Json json = json11::Json::parse(real_content, err);
	if (!err.empty())
	{
		LOG_ERROR << "Parse config file(" << filename << ") error: " << err << ENDL;
		return false;
	}

	Json_FillObject(json, *this);

	return true;
}

bool ServerConfig::HaveLocalService(const std::string & serv_type_name)
{
	auto it = type_to_services.find(serv_type_name);
	if (it == type_to_services.end())
	{
		return false;
	}

	for (const auto & pr : it->second)
	{
		if (pr.second->is_local_service)
		{
			return true;
		}
	}

	return false;
}

void Json_FillObject(const json11::Json & json, ServerConfig & obj)
{
	JSON_FILLFIELD_WITH_DEFAULT(res_path, std::string("./res"));
	JSON_FILLFIELD_WITH_DEFAULT(thread_num, 2);
	if (obj.res_path.empty() || *(obj.res_path.end() - 1) != '/' || *(obj.res_path.end() - 1) != '\\')
	{
		obj.res_path.push_back('/');
	}
	obj.thread_num = std::max(1, obj.thread_num);

	std::string listen_service; // 服务监听地址
	Json_FillField(json, "listen_service", listen_service);
	obj.listen_service = std::make_shared<NetAddrInfo>();
	if (!obj.listen_service->ParseFormString(listen_service))
	{
		obj.listen_service.reset();
	}

	std::string listen_manager; // 管理监听地址
	Json_FillField(json, "listen_manager", listen_manager);
	obj.listen_manager = std::make_shared<NetAddrInfo>();
	if (!obj.listen_manager->ParseFormString(listen_manager))
	{
		obj.listen_manager.reset();
	}

	std::unordered_map<std::string, std::string> service; // 服务信息
	Json_FillField(json, "service", service);
	for (auto & it : service)
	{
		int32_t sid = sframe::StrToAny<int32_t>(it.first);
		if (sid <= 0)
		{
			continue;
		}

		std::shared_ptr<ServiceInfo> s_info = std::make_shared<ServiceInfo>();
		if (!s_info->ParseFormString(it.second))
		{
			continue;
		}
		s_info->sid = sid;
		assert(!s_info->service_type_name.empty());

		obj.services[sid] = s_info;
		obj.type_to_services[s_info->service_type_name][sid] = s_info;
	}

	std::unordered_map<std::string, std::vector<std::string>> listen_custom; // 自定义监听地址(服务类型->地址列表)
	Json_FillField(json, "listen_custom", listen_custom);
	for (auto & pr : listen_custom)
	{
		std::string serv_type_name = pr.first;
		assert(!serv_type_name.empty());

		// 本地服务中必须有该服务
		if (!obj.HaveLocalService(serv_type_name))
		{
			continue;
		}

		std::vector<ListenAddrInfo> * vec_addr_info = nullptr;
		for (const auto & addr : pr.second)
		{
			ListenAddrInfo listen_info;
			if (!listen_info.ParseFormString(addr))
			{
				continue;
			}

			if (!vec_addr_info)
			{
				vec_addr_info = &obj.listen_custom[serv_type_name];
			}
			vec_addr_info->push_back(listen_info);
		}
	}
}