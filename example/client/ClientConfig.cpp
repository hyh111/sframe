
#include "ClientConfig.h"
#include "conf/TableReader.h"

FILL_TABLECONFIG(ClientConfig)
{
	TABLE_FILLFIELD(client_id);
	TABLE_FILLFIELD(server_ip);
	TABLE_FILLFIELD(server_port);
	TABLE_FILLFIELD(text);
	TABLE_FILLFIELD(test);
}