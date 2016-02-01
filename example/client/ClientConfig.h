
#pragma once
#ifndef __CLIENT_CONFIG_H__
#define __CLIENT_CONFIG_H__

#include <inttypes.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "conf/ConfigDef.h"
#include "ConfigType.h"

TABLECONFIG(ClientConfig)
{
	CONFIGINFO(kConfigType_ClientConfig, "client_config.csv");

	typedef int32_t KeyType;

	KeyType GetKey() const
	{
		return client_id;
	}

	int32_t client_id;
	std::string server_ip;
	uint16_t server_port;
	std::vector<std::string> text;
	std::unordered_map<int32_t, std::string> test;
};

#endif