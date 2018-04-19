
#include "serv/ServiceDispatcher.h"
#include "ClientSession.h"
#include "GateService.h"
#include "../ssproto/SSMsg.h"
#include "util/Serialization.h"
#include "util/Log.h"

using namespace sframe;

// 接收到数据
// 返回剩余多少数据
int32_t ClientSession::OnReceived(char * data, int32_t len)
{
	char * p = data;
	int32_t surplus = len;

	while (surplus > 0)
	{
		size_t msg_size = 0;
		StreamReader msg_size_reader(p, surplus);
		if (!msg_size_reader.ReadSizeField(msg_size) ||
			msg_size_reader.GetNotReadLength() < msg_size)
		{
			break;
		}

		p += msg_size_reader.GetReadedLength();
		surplus -= (int32_t)msg_size_reader.GetReadedLength();

		std::shared_ptr<std::vector<char>> data = std::make_shared<std::vector<char>>(msg_size);
		if (msg_size > 0)
		{
			memcpy(&(*data)[0], p, msg_size);
			p += msg_size;
			surplus -= msg_size;
		}

		std::shared_ptr<ClientSession> session = shared_from_this();
		ServiceDispatcher::Instance().SendInsideServiceMsg(0, _gate_service->GetServiceId(), 0, kGateMsg_SessionRecvData, session, data);
	}

	return surplus;
}

// Socket关闭
// by_self: true表示主动请求的关闭操作
void ClientSession::OnClosed(bool by_self, sframe::Error err)
{
	std::shared_ptr<ClientSession> session = shared_from_this();
	ServiceDispatcher::Instance().SendInsideServiceMsg((int32_t)0, _gate_service->GetServiceId(), 0, (uint16_t)kGateMsg_SessionClosed, session);
}


ClientSession::ClientSession(GateService * gate_service, int64_t session_id, const std::shared_ptr<sframe::TcpSocket> & sock)
	: _gate_service(gate_service), _sock(sock), _session_id(session_id), _cur_work_sid(0)
{
	_sock->SetMonitor(this);
	_sock->StartRecv();
}

void ClientSession::StartClose()
{
	assert(_sock != nullptr);
	_sock->Close();
}

void ClientSession::EnterWorkService(int32_t sid)
{
	if (_cur_work_sid > 0 || sid <= 0)
	{
		return;
	}

	_cur_work_sid = sid;
	int32_t gate_sid = _gate_service->GetServiceId();
	_gate_service->SendServiceMsg(_cur_work_sid, _session_id, (uint16_t)kWorkMsg_EnterWorkService, gate_sid, _session_id);
}

void ClientSession::HandleClosed()
{
	_sock.reset();
	int32_t gate_sid = _gate_service->GetServiceId();
	ServiceDispatcher::Instance().SendServiceMsg(_gate_service->GetServiceId(), _cur_work_sid, _session_id,
		(uint16_t)kWorkMsg_QuitWorkService, gate_sid, _session_id);
}

void ClientSession::SendToWorkService(const std::shared_ptr<std::vector<char>> & data)
{
	if (_cur_work_sid <= 0)
	{
		assert(false);
		LOG_INFO << "ClientSession not in WorkService" << ENDL;
		return;
	}

	WorkMsg_ClientData msg;
	msg.gate_sid = _gate_service->GetServiceId();
	msg.session_id = _session_id;
	msg.client_data = data;
	ServiceDispatcher::Instance().SendServiceMsg(_gate_service->GetServiceId(), _cur_work_sid, _session_id, (uint16_t)kWorkMsg_ClientData, msg);
}

void ClientSession::SendToClient(const std::shared_ptr<std::vector<char>> & data)
{
	this->_sock->Send(&(*data)[0], (int32_t)data->size());
}