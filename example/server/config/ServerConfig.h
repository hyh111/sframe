
#ifndef PUBDEF_SERVER_CONFIG_H
#define PUBDEF_SERVER_CONFIG_H

#include <inttypes.h>
#include <string>
#include <vector>
#include "conf/ConfigDef.h"
#include "util/Singleton.h"

JSONCONFIG(ServerInfo)
{
	std::string ip;
	uint16_t port;
	std::string key;
};

JSONCONFIG(ServerConfig) : public sframe::singleton<ServerConfig>
{
	bool Load(const std::string & filename);

	std::string res_path;       // 资源目录
	int32_t thread_num;         // 线程数量
	std::vector<int32_t> local_service;  // 本地开启服务
	ServerInfo service_listen;    // 服务监听地址
	ServerInfo client_listen;     // 客户端监听地址
	std::vector<ServerInfo> remote_server;   // 要连接的远程服务器
};


#endif
