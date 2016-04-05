
#include "ConfigManager.h"
#include "conf/csv.h"
#include "conf/TableLoader.h"


using namespace sframe;

void ConfigManager::RegistAllConfig()
{
	RegistMapConfig<TableLoader<CSV>, ClientConfig::KeyType, ClientConfig>();
}