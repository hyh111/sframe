
#include <algorithm>
#include "ClientManager.h"
#include "serv/ServiceDispatcher.h"
#include "util/Log.h"
#include "../ssproto/SSMsg.h"

using namespace sframe;

// 连接通知
void ClientManager::OnAccept(std::shared_ptr<sframe::TcpSocket> socket, sframe::Error err)
{
	if (err)
	{
		LOG_ERROR << "ClientManager accept error(" << err.Code() << "): " << ErrorMessage(err).Message() << ENDL;
		return;
	}

	int32_t gate = ChooseGate();
	if (gate <= 0)
	{
		LOG_ERROR << "New connection builded, but have no gate" << ENDL;
		return;
	}

	ServiceDispatcher::Instance().SendInsideServiceMsg(0, gate, kGateMsg_NewSession, socket);
}

// 停止
void ClientManager::OnClosed(sframe::Error err)
{
	_acceptor.reset();

	if (err)
	{
		LOG_ERROR << "ClientManager closed with error(" << err.Code() << "): " << ErrorMessage(err).Message() << ENDL;
	}
}

// 更新网关服务的当前连接数
void ClientManager::UpdateGateServiceInfo(int32_t sid, int32_t cur_session_num)
{
	assert(sid > 0);
	AutoLock<Lock> l(_lock);
	
	GateServiceInfo * info = nullptr;
	for (auto & i : _gate_info)
	{
		if (i.sid == sid)
		{
			info = &i;
			break;
		}
	}

	if (info == nullptr)
	{
		_gate_info.push_back(GateServiceInfo{ sid, cur_session_num });
	}
	else
	{
		info->session_num = cur_session_num;
	}

	// 排序
	std::sort(_gate_info.begin(), _gate_info.end());
}

// 开始
bool ClientManager::Start(const std::string & ip, uint16_t port)
{
	_acceptor = TcpAcceptor::Create(ServiceDispatcher::Instance().GetIoService());
	assert(_acceptor);
	_acceptor->SetMonitor(this);
	Error err = _acceptor->Start(SocketAddr(ip.c_str(), port));
	if (err)
	{
		_acceptor.reset();
		LOG_ERROR << "ClientManager listen " << ip << ":" << port << "error(" << err.Code() << "): " << ErrorMessage(err).Message() << ENDL;
		return false;
	}

	LOG_INFO << "Start listen port " << port << ENDL;
	return true;
}

// 选择一个Gate服务
int32_t ClientManager::ChooseGate()
{
	if (_gate_info.empty())
	{
		return -1;
	}

	int32_t gate = _gate_info[0].sid;
	_gate_info[0].session_num++;
	// 排序
	std::sort(_gate_info.begin(), _gate_info.end());

	return gate;
}