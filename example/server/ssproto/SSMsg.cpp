
#include "SSMsg.h"

SERIALIZE2(GateMsg_SendToClient, session_id, client_data)

SERIALIZE3(WorkMsg_ClientData, gate_sid, session_id, client_data)

