
#pragma once
#ifndef __CLIENT_CONFIG_H__
#define __CLIENT_CONFIG_H__

#include "conf/ConfigDef.h"
#include "ConfigType.h"

struct ClientConfig
{
	STATIC_MAP_CONFIG(int32_t, ClientConfig, client_id, kConfigId_ClientConfig)

	void Fill(sframe::TableReader & reader);

	bool PutIn(std::map<int32_t, std::shared_ptr<ClientConfig>> & m);

	int32_t client_id;
	std::string server_ip;
	uint16_t server_port;
	std::vector<std::string> text;
	std::unordered_map<int32_t, std::string> test;
};

#endif