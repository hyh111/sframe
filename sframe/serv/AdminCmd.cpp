
#include "ServiceDispatcher.h"
#include "AdminCmd.h"
#include "../util/StringHelper.h"

using namespace sframe;

const std::string & AdminCmd::GetCmdParam(const std::string & k) const
{
	auto it = _cmd_param.find(k);
	if (it == _cmd_param.end())
	{
		static std::string empty_str;
		return empty_str;
	}

	return it->second;
}

bool AdminCmd::Parse(const std::shared_ptr<sframe::HttpRequest> & http_req)
{
	_http_req = http_req;
	_cmd_name = TrimLeft(http_req->GetRequestUrl(), '/');
	if (_cmd_name.empty())
	{
		return false;
	}

	_cmd_param = sframe::Http::ParseHttpParam(http_req->GetRequestParam());

	return true;
}

void AdminCmd::SendResponse(const std::string & data) const
{
	ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, 0, kProxyServiceMsgId_SendAdminCommandResponse, _admin_session_id, data, _http_req);
}

std::string AdminCmd::ToString() const
{
	std::ostringstream oss;
	oss << _cmd_name << '?';
	for (std::unordered_map<std::string, std::string>::const_iterator it = _cmd_param.begin(); it != _cmd_param.end(); it++)
	{
		if (it->first.empty())
		{
			continue;
		}

		if (it != _cmd_param.begin())
		{
			oss << '&';
		}
		oss << it->first << '=' << it->second;
	}
	return oss.str();
}

