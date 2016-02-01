
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include "util/FileHelper.h"
#include "config/ServerConfig.h"
#include "serv/ServiceDispatcher.h"
#include "gateservice/GateService.h"
#include "gateservice/ClientManager.h"
#include "workservice/WorkService.h"
#include "util/Log.h"
#include "util/TimeHelper.h"

using namespace sframe;

bool Start()
{
	// 注册本地服务
	for (int32_t sid : ServerConfig::Instance().local_service)
	{
		Service * service = nullptr;

		if (sid >= kSID_GateServiceBegin && sid <= kSID_GateServiceEnd)
		{
			service = ServiceDispatcher::Instance().RegistService<GateService>(sid);
		}
		else if (sid >= kSID_WorkServiceBegin && sid <= kSID_WorkServiceEnd)
		{
			service = ServiceDispatcher::Instance().RegistService<WorkService>(sid);
		}

		if (service == nullptr)
		{
			LOG_ERROR << "regist service " << sid << " failure" << ENDL;
			return false;
		}
	}

	// 注册远程服务器
	for (const ServerInfo & server_info : ServerConfig::Instance().remote_server)
	{
		if (!ServiceDispatcher::Instance().RegistRemoteServer(server_info.ip, server_info.port, server_info.key))
		{
			LOG_ERROR << "regist remote server(" << server_info.ip << ":" << server_info.port << ") failure" << ENDL;
			return false;
		}
	}

	if (!ServerConfig::Instance().service_listen.ip.empty())
	{
		ServiceDispatcher::Instance().SetListenAddr(
			ServerConfig::Instance().service_listen.ip,
			ServerConfig::Instance().service_listen.port,
			ServerConfig::Instance().service_listen.key);
	}

	

	if (!ServiceDispatcher::Instance().Start(ServerConfig::Instance().thread_num))
	{
		LOG_ERROR << "start server failure" << ENDL;
		return false;
	}

	if (!ServerConfig::Instance().client_listen.ip.empty())
	{
		ClientManager::Instance().Start(ServerConfig::Instance().client_listen.ip, ServerConfig::Instance().client_listen.port);
	}

	return true;
}

int main(int argc, char * argv[])
{
	std::string config_name;

	if (argc < 2)
	{
		config_name = "./" + FileHelper::RemoveExtension(FileHelper::GetFileName(argv[0])) + ".conf";
	}
	else
	{
		config_name = argv[1];
	}
	
	SET_LOG_DIR("./" + FileHelper::RemoveExtension(FileHelper::GetFileName(config_name)) + "_log");

	if (!ServerConfig::Instance().Load(config_name))
	{
		return -1;
	}

	if (!Start())
	{
		return -1;
	}

	//getchar();
	while (true)
	{
		//sframe::TimeHelper::ThreadSleep(2000);
	}

	ServiceDispatcher::Instance().Stop();

	return 0;
}