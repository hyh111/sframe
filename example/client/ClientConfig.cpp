
#include "ClientConfig.h"
#include "conf/TableReader.h"

void ClientConfig::Fill(sframe::TableReader & reader)
{
	FILLFIELD(client_id);
	FILLFIELD(server_ip);
	FILLFIELD(server_port);
	FILLFIELD(text);
	FILLFIELD(test);
}
