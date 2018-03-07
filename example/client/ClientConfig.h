
#ifndef __CLIENT_CONFIG_H__
#define __CLIENT_CONFIG_H__

#include "conf/ConfigDef.h"
#include "ConfigType.h"

struct ClientConfig
{
	void Fill(sframe::TableReader & reader);

	KEY_FIELD(int32_t, client_id);

	int32_t client_id;
	std::string server_ip;
	uint16_t server_port;
	std::vector<std::string> text;
	std::unordered_map<int32_t, std::string> test;
};


MAP_CONFIG_MODULE(ClientConfigModule, int32_t, ClientConfig, kConfigId_ClientConfig);


#endif