
#include "ConfigManager.h"
#include "conf/csv.h"
#include "conf/TableLoader.h"


using namespace sframe;

bool ConfigManager::LoadAll()
{
	auto client_config = LoadMap<TableLoader<CSV>, ClientConfig::KeyType, ClientConfig>();

	return client_config != nullptr;
}