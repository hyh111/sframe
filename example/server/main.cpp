
#ifdef __GNUC__
#include <signal.h>
#endif

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
#include "workservice/WorkService.h"
#include "util/Log.h"
#include "util/TimeHelper.h"
#include "util/ObjectFactory.h"

using namespace sframe;

bool Start()
{
	std::unordered_map<std::string, std::set<int32_t>> local_service;

	// 注册所有服务
	for (const auto & pr : ServerConfig::Instance().services)
	{
		auto serv_info = pr.second;

		if (serv_info->is_local_service)
		{
			Service * serv = GlobalObjFactory::Instance().Create<Service>(serv_info->service_type_name);
			if (serv == nullptr)
			{
				LOG_ERROR << "Create service failure|" << serv_info->service_type_name << "|" << serv_info->sid << std::endl;
				continue;
			}

			if (!ServiceDispatcher::Instance().RegistService(serv_info->sid, serv))
			{
				delete serv;
				LOG_ERROR << "Regist local service failure|" << serv_info->service_type_name << "|" << serv_info->sid << std::endl;
				continue;
			}

			LOG_INFO << "Regist local service success|" << serv_info->service_type_name << "|" << serv_info->sid << std::endl;
			local_service[serv_info->service_type_name].insert(serv_info->sid);
		}
		else
		{
			if (!ServiceDispatcher::Instance().RegistRemoteService(serv_info->sid, serv_info->remote_addr.ip, serv_info->remote_addr.port))
			{
				LOG_ERROR << "Regist remote service failure|" << serv_info->service_type_name << "|" << serv_info->sid << std::endl;
				continue;
			}

			LOG_INFO << "Regist remote service success|" << serv_info->service_type_name << "|" << serv_info->sid << std::endl;
		}
	}

	if (ServerConfig::Instance().listen_service)
	{
		ServiceDispatcher::Instance().SetServiceListenAddr(
			ServerConfig::Instance().listen_service->ip,
			ServerConfig::Instance().listen_service->port);
	}

	for (const auto & pr : ServerConfig::Instance().listen_custom)
	{
		auto it = local_service.find(pr.first);
		if (it == local_service.end() || it->second.empty())
		{
			continue;
		}

		for (const auto & addr_info : pr.second)
		{
			ServiceDispatcher::Instance().SetCustomListenAddr(addr_info.desc, addr_info.addr.ip, addr_info.addr.port, it->second);
		}
	}

	if (!ServiceDispatcher::Instance().Start(ServerConfig::Instance().thread_num))
	{
		LOG_ERROR << "start server failure" << ENDL;
		return false;
	}

	return true;
}

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <CONFIG_FILE>\n", argv[0]);
		return -1;
	}

	std::string config_name = argv[1];
	INITIALIZE_LOG("./" + FileHelper::RemoveExtension(FileHelper::GetFileName(config_name)) + "_log", "");
	if (!ServerConfig::Instance().Load(config_name))
	{
		return -1;
	}

#ifdef __GNUC__

	sigset_t wai_sig_set;
	sigemptyset(&wai_sig_set);
	sigaddset(&wai_sig_set, SIGQUIT);
	int r = pthread_sigmask(SIG_BLOCK, &wai_sig_set, nullptr);
	if (r != 0)
	{
		LOG_ERROR << "pthread_sigmask error(" << r << ") : " << strerror(r) << ENDL;
		return -1;
	}

#endif

	if (!Start())
	{
		return -1;
	}

#ifdef __GNUC__

	while (true)
	{
		int sig;
		int r = sigwait(&wai_sig_set, &sig);
		if (r == 0)
		{
			break;
		}
	}

#else

	getchar();

#endif

	ServiceDispatcher::Instance().Stop();

	return 0;
}