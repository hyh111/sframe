
#include "ClientConfig.h"
#include "conf/TableReader.h"

void ClientConfig::Fill(sframe::TableReader & reader)
{
	TBL_FILLFIELD(client_id);
	TBL_FILLFIELD(server_ip);
	TBL_FILLFIELD(server_port);
	TBL_FILLFIELD(text);
	TBL_FILLFIELD(test);
}
