
#ifndef __SSMSG_H__
#define __SSMSG_H__

#include <inttypes.h>
#include <vector>
#include <memory>
#include "util/Serialization.h"

enum GateMsgId : uint16_t
{
	kGateMsg_Start = 1,

	kGateMsg_RegistWorkService = kGateMsg_Start,
	kGateMsg_NewSession,
	kGateMsg_SessionClosed,
	kGateMsg_SessionRecvData,
	kGateMsg_SendToClient,

	kGateMsg_End = 100
};

enum WorkMsgId : uint16_t
{
	kWorkMsg_Start = 101,

	kWorkMsg_ClientData = kWorkMsg_Start,
	kWorkMsg_EnterWorkService,
	kWorkMsg_QuitWorkService,

	kWorkMsg_End
};

SERIALIZABLE_STRUCT(GateMsg_SendToClient)
{
	int32_t session_id;
	std::shared_ptr<std::vector<char>> client_data;
};

SERIALIZABLE_STRUCT(WorkMsg_ClientData)
{
	int32_t gate_sid;
	int32_t session_id;
	std::shared_ptr<std::vector<char>> client_data;
};

#endif