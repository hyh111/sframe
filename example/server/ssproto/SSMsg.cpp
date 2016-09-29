
#include "SSMsg.h"

DEFINE_SERIALIZE_OUTER(GateMsg_SendToClient, session_id, client_data)

DEFINE_SERIALIZE_OUTER(WorkMsg_ClientData, gate_sid, session_id, client_data)

