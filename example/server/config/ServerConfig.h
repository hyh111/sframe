
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
	uint16_t port;            // ¶Ë¿Ú
};

struct ListenAddrInfo
{
	bool ParseFormString(const std::string & data);

	std::string desc;         // ÃèÊö£¬±ãÓÚÊ¶±ð
	NetAddrInfo addr;         // µØÖ·
};

struct ServiceInfo
{
	bool ParseFormString(const std::string & data);

	int32_t sid;
	std::string service_type_name;   // ·þÎñÀàÐÍÃû³Æ
	bool is_local_service;           // true Îª±¾µØ·þÎñ£¬falseÎªÔ¶³Ì·þÎñ
	NetAddrInfo remote_addr;         // Ô¶³ÌµØÖ·£¬½öµ±local_serviceÎªfalseÊÇÓÐÐ§
};

struct ServerConfig : public sframe::singleton<ServerConfig>
{
	bool Load(const std::string & filename);

	void Fill(const json11::Json & reader);

	bool HaveLocalService(const std::string & serv_type_name);

	std::string server_name;                  // ·þÎñÆ÷Ãû³Æ
	std::string res_path;                     // ×ÊÔ´Ä¿Â¼
	int32_t thread_num;                       // Ïß³ÌÊýÁ¿
	std::shared_ptr<NetAddrInfo> listen_service;                                 // Ô¶³Ì·þÎñ¼àÌýµØÖ·
	std::shared_ptr<NetAddrInfo> listen_admin;                                 // ¹ÜÀíµØÖ·
	std::unordered_map<int32_t, std::shared_ptr<ServiceInfo>> services;          // ·þÎñÐÅÏ¢£¨sid -> ·þÎñÐÅÏ¢£©
	std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<ServiceInfo>>> type_to_services;  // ÀàÐÍ->¸ÃÀàÐÍËùÓÐ·þÎñÐÅÏ¢
	std::unordered_map<std::string, std::vector<ListenAddrInfo>> listen_custom;  // ×Ô¶¨Òå¼àÌý
};


#endif
