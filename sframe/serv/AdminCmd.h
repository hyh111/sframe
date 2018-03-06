
#ifndef SFRAME_ADMIN_CMD_H
#define SFRAME_ADMIN_CMD_H

#include <inttypes.h>
#include <string>
#include <vector>
#include <functional>
#include "../util/Http.h"

namespace sframe {

// ¹ÜÀíÃüÁî
class AdminCmd
{
public:

	AdminCmd(int32_t admin_session_id) : _admin_session_id(admin_session_id) {}

	~AdminCmd() {}

	const std::string & GetCmdName() const
	{
		return _cmd_name;
	}

	const std::unordered_map<std::string, std::string> & GetCmdParam() const
	{
		return _cmd_param;
	}

	const std::string & GetCmdParam(const std::string & k) const;

	bool Parse(const std::shared_ptr<sframe::HttpRequest> & http_req);

	void SendResponse(const std::string & data) const;

	std::string ToString() const;

private:
	int32_t _admin_session_id;
	std::string _cmd_name;
	std::unordered_map<std::string, std::string> _cmd_param;
	std::shared_ptr<sframe::HttpRequest> _http_req;
};

// ¹ÜÀíÃüÁî´¦Àí·½·¨
typedef std::function<void(const AdminCmd &)> AdminCmdHandleFunc;

}

#endif