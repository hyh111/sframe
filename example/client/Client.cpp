
#include <assert.h>
#include "Client.h"
#include "ClientManager.h"
#include "ConfigManager.h"
#include "util/Log.h"
#include "util/Serialization.h"
#include "util/TimeHelper.h"

Client::TimerRouine Client::kStateRoutine[kState_Count] = {
	&Client::OnTimer_WaitConnect,
	nullptr,
	&Client::OnTimer_Working
};

Client::Client(int32_t id, ClientManager * mgr)
	: _id(id), _mgr(mgr), _state(kState_WaitConnect), _server_port(0), _text_index(0), _count(0) 
{
	_log_name = std::to_string(_id);
}

void Client::Init()
{
	RegistTimer(1000, &Client::OnTimer);
}

void Client::Close()
{
	if (_sock && _sock->IsOpen())
	{
		_sock->Close();
	}
	else
	{
		_mgr->CloseClient(_id);
	}
}

// 接收到数据
// 返回剩余多少数据
int32_t Client::OnReceived(char * data, int32_t len)
{
	std::string s(data, len);
	FLOG(_log_name) << "Client(" << _id << ") recv data: " << s << ENDL;
	return 0;
}

// Socket关闭
// by_self: true表示主动请求的关闭操作
void Client::OnClosed(bool by_self, sframe::Error err)
{
	if (err)
	{
		LOG_INFO << "Client(" << _id << ") closed with error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
	}
	else
	{
		LOG_INFO << "Client(" << _id << ") closed" << ENDL;
	}

	_mgr->CloseClient(_id);
}

// 连接操作完成
void Client::OnConnected(sframe::Error err)
{
	if (err)
	{
		LOG_ERROR << "Client(" << _id << ") connect to " << _server_ip << ":" << _server_port
			<< " error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
		_sock.reset();
		_mgr->CloseClient(_id);
		return;
	}

	LOG_INFO << "Client(" << _id << ") connect to " << _server_ip << ":" << _server_port << " success" << ENDL;
	_state = kState_Working;
	_sock->StartRecv();
}

int32_t Client::OnTimer()
{
	if (_state >= 0 && _state < kState_Count)
	{
		if (kStateRoutine[_state])
		{
			(this->*kStateRoutine[_state])();
		}
	}

	return 1000;
}

void Client::OnTimer_WaitConnect()
{
	assert(_sock == nullptr);
	_sock = sframe::TcpSocket::Create(_mgr->GetIoService());
	auto config = ConfigManager::GetConfigSet()->GetMapConfigItem<ClientConfig>(_id);
	assert(config != nullptr);
	_server_ip = config->server_ip;
	_server_port = config->server_port;
	_sock->SetMonitor(this);
	_sock->SetTcpNodelay(true);
	_sock->Connect(sframe::SocketAddr(_server_ip.c_str(), _server_port));
	_state = kState_Connecting;
}

void Client::OnTimer_Working()
{
	assert(_sock);
	auto config = ConfigManager::GetConfigSet()->GetMapConfigItem<ClientConfig>(_id);
	assert(config != nullptr);

	if (config->text.empty())
	{
		return;
	}

	if (_text_index >= (int32_t)config->text.size())
	{
		_text_index = 0;
	}

	int64_t cur_time = sframe::TimeHelper::GetEpochMilliseconds();
	uint16_t msg_size = 0;
	char buf[1024];
	sframe::StreamWriter stream_writer(buf + sizeof(msg_size), 1024 - sizeof(msg_size));
	if (!sframe::AutoEncode(stream_writer, _id, _count, cur_time, config->text[_text_index]))
	{
		LOG_ERROR << "Encode msg error" << ENDL;
		return;
	}

	msg_size = (uint16_t)stream_writer.GetStreamLength();
	sframe::StreamWriter size_writer(buf, sizeof(msg_size));
	if (!sframe::AutoEncode(size_writer, msg_size))
	{
		LOG_ERROR << "Encode msg error" << ENDL;
		return;
	}

	_count++;
	_text_index++;

	_sock->Send(buf, msg_size + sizeof(msg_size));
}