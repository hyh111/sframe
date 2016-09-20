
#ifndef __USER_H__
#define __USER_H__

#include <inttypes.h>
#include <vector>
#include <string>
#include "../ssproto/SSMsg.h"

class User
{
public:
	User(const User &) = delete;
	User & operator=(const User&) = delete;

	User(int32_t work_sid, int32_t gate_sid, int64_t session_id);

	~User() {}

	void OnClientData(const WorkMsg_ClientData & msg);

private:
	int32_t _work_sid;
	int64_t _session_id;
	int32_t _gate_sid;
	std::string _log_name;
};

#endif
