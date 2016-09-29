
#include <stdio.h>
#include "User.h"
#include "util/Serialization.h"
#include "util/Log.h"
#include "serv/ServiceDispatcher.h"
#include "../ssproto/SSMsg.h"
#include "util/TimeHelper.h"

using namespace sframe;

User::User(int32_t work_sid, int32_t gate_sid, int64_t session_id) 
	: _work_sid(work_sid), _gate_sid(gate_sid), _session_id(session_id)
{
	char file_name[256];
	sprintf(file_name, "user_%d_%lld.txt", _gate_sid, _session_id);
	_log_name = file_name;
}

void User::OnClientData(const WorkMsg_ClientData & msg)
{
	auto & data = (*(msg.client_data));

	int32_t client_id;
	int32_t count;
	int64_t send_time;
	std::string text;
	uint32_t len = (uint32_t)data.size();
	StreamReader stream_reader(len > 0 ? &data[0] : nullptr, len);
	if (!AutoDecode(stream_reader, client_id, count, send_time, text))
	{
		LOG_ERROR << "decode client data error" << ENDL;
		return;
	}

	FLOG(_log_name) << client_id << "|" << count << "|" << send_time << "|" << TimeHelper::GetEpochMilliseconds() << "|" << text << ENDL;

	char resp_text[1024];
	sprintf(resp_text, "Client %d, you are in gate(%d), and your sessionid id is %lld", client_id, _gate_sid, _session_id);
	int resp_text_len = (int)strlen(resp_text);

	GateMsg_SendToClient resp_msg;
	resp_msg.session_id = _session_id;
	resp_msg.client_data = std::make_shared<std::vector<char>>(resp_text_len);
	memcpy(&(*resp_msg.client_data)[0], resp_text, resp_text_len);
	ServiceDispatcher::Instance().SendServiceMsg(_work_sid, _gate_sid, 0, kGateMsg_SendToClient, resp_msg);
}