
#ifdef __GNUC__
#include <signal.h>
#endif

#include <stdio.h>
#include <string.h>
#include <thread>
#include <iostream>
#include "ConfigManager.h"
#include "ClientManager.h"
#include "util/Log.h"

bool g_running = true;

void Exec()
{
	while (g_running)
	{
		ClientManager::Instance().Update();
	}
}

int main()
{
	INITIALIZE_LOG("./client_log", "");

	if (!ConfigManager::InitializeConfig("./client_data"))
	{
		LOG_ERROR << "Config error, quit now..." << std::endl;
		return -1;
	}

	if (!ClientManager::Instance().Init())
	{
		LOG_ERROR << "Init ClientManager error" << ENDL;
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

	std::thread t(Exec);

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

	ClientManager::Instance().Close();
	g_running = false;
	t.join();

	return 0;
}