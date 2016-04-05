
#pragma once
#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include "util/Singleton.h"
#include "conf/ConfigSet.h"
#include "ConfigType.h"
#include "ClientConfig.h"

class ConfigManager : public sframe::ConfigSet , public sframe::singleton<ConfigManager>
{
public:
	ConfigManager() : ConfigSet(kConfigType_Max) {}

private:
	void RegistAllConfig() override;

};

#endif