
#ifndef PUBDEF_SERVER_CONFIG_H
#define PUBDEF_SERVER_CONFIG_H

#include <inttypes.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include "conf/ConfigDef.h"
#include "util/Singleton.h"

struct NetAddrInfo
{
	bool ParseFormString(const std::string & data);

	std::string ip;           // IP
	uint16_t port;            // 端口
};

struct ListenAddrInfo
{
	bool ParseFormString(const std::string & data);

	std::string desc;         // 描述，便于识别
	NetAddrInfo addr;         // 地址
};

struct ServiceInfo
{
	bool ParseFormString(const std::string & data);

	int32_t sid;
	std::string service_type_name;   // 服务类型名称
	bool is_local_service;           // true 为本地服务，false为远程服务
	NetAddrInfo remote_addr;         // 远程地址，仅当local_service为false是有效
};

struct ServerConfig : public sframe::singleton<ServerConfig>
{
	bool Load(const std::string & filename);

	void Fill(json11::Json & reader);

	bool HaveLocalService(const std::string & serv_type_name);

	std::string res_path;                     // 资源目录
	int32_t thread_num;                       // 线程数量
	std::shared_ptr<NetAddrInfo> listen_service;                                 // 远程服务监听地址
	std::shared_ptr<NetAddrInfo> listen_manager;                                 // 管理地址
	std::unordered_map<int32_t, std::shared_ptr<ServiceInfo>> services;          // 服务信息（sid -> 服务信息）
	std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<ServiceInfo>>> type_to_services;  // 类型->该类型所有服务信息
	std::unordered_map<std::string, std::vector<ListenAddrInfo>> listen_custom;  // 自定义监听
};


#endif
