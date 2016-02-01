
#include <stdio.h>
#include "User.h"
#include "util/Serialization.h"
#include "util/Log.h"
#include "serv/ServiceDispatcher.h"
#include "../ssproto/SSMsg.h"

using namespace sframe;

User::User(int32_t work_sid, int32_t gate_sid, int32_t session_id) 
	: _work_sid(work_sid), _gate_sid(gate_sid), _session_id(session_id)
{
	char file_name[256];
	sprintf(file_name, "user_%d_%d.txt", _gate_sid, _session_id);
	_log_name = file_name;
}

void User::OnClientMsg(const std::vector<char> & data)
{
	int32_t client_id;
	int32_t count;
	std::string text;
	uint32_t len = (uint32_t)data.size();
	StreamReader stream_reader(len > 0 ? &data[0] : nullptr, len);
	if (!AutoDecode(stream_reader, client_id, count, text))
	{
		LOG_ERROR << "decode client data error" << ENDL;
		return;
	}

	FLOG(_log_name) << client_id << ": " << count << " -> " << text << ENDL;

	char msg[1024];
	sprintf(msg, "Client %d, you are in gate(%d), and your sessionid id is %d", client_id, _gate_sid, _session_id);
	int msg_len = (int)strlen(msg);

	GateMsg_SendToClient resp_msg;
	resp_msg.session_id = _session_id;
	resp_msg.client_data = std::make_shared<std::vector<char>>(msg_len);
	memcpy(&(*resp_msg.client_data)[0], msg, msg_len);
	ServiceDispatcher::Instance().SendServiceMsg(_work_sid, _gate_sid, kGateMsg_SendToClient, resp_msg);
}