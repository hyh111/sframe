
#include "ServiceDispatcher.h"
#include "ServiceSession.h"
#include "ProxyService.h"
#include "../util/Log.h"
#include "../util/TimeHelper.h"
#include "../util/md5.h"

using namespace sframe;

ServiceSession::ServiceSession(int32_t id, ProxyService * proxy_service, const std::string & remote_ip, uint16_t remote_port, const std::string & remote_key)
	: _session_id(id), _remote_ip(remote_ip), _remote_port(remote_port), _proxy_service(proxy_service), 
	_state(kSessionState_WaitConnect),  _remote_key(remote_key),  _reconnect(true)
{
	assert(!remote_ip.empty() && proxy_service);
}

ServiceSession::ServiceSession(int32_t id, ProxyService * proxy_service, std::shared_ptr<sframe::TcpSocket> & sock)
	: _session_id(id), _socket(sock), _state(kSessionState_WaitAuth), _proxy_service(proxy_service), _reconnect(false)
{
	assert(sock != nullptr && proxy_service);
}


void ServiceSession::Init()
{
	if (_socket == nullptr)
	{
		StartConnectTimer(0);
	}
	else
	{
		_socket->SetMonitor(this);
		// 开始接收数据
		_socket->StartRecv();
	}
}


void ServiceSession::Close()
{
	bool send_close = true;

	_reconnect = false;

	if (_socket != nullptr)
	{
		send_close = _socket->Close() == false;
	}

	if (send_close)
	{
		std::shared_ptr<InsideServiceMessage<int32_t>> msg = std::make_shared<InsideServiceMessage<int32_t>>(_session_id);
		msg->dest_sid = 0;
		msg->src_sid = 0;
		msg->msg_id = kProxyServiceMsgId_SessionClosed;
		ServiceDispatcher::Instance().SendMsg(0, msg);
	}
}

// 尝试释放
bool ServiceSession::TryFree()
{
	if (!_reconnect)
	{
		return true;
	}

	_socket.reset();
	_state = kSessionState_WaitConnect;
	// 开启连接定时器
	StartConnectTimer(kReconnectInterval);

	return false;
}

// 连接操作完成处理
void ServiceSession::DoConnectCompleted(bool success)
{
	if (!success)
	{
		_socket.reset();
		_state = kSessionState_WaitConnect;
		// 开启连接定时器
		StartConnectTimer(kReconnectInterval);
		return;
	}

	_state = kSessionState_Authing;
	// 发送验证信息
	bool sendresult = SendAuthMessage();
	if (!sendresult)
	{
		assert(false);
		_socket->Close();
	}
}

// 接受数据
void ServiceSession::DoRecvData(std::vector<char> & data)
{
	// 空为心跳包
	if (data.empty())
	{
		return;
	}

	switch (_state)
	{
	case kSessionState_Running:
		ReceiveData_Running(data);
		break;

	case ServiceSession::kSessionState_WaitAuth:
		ReceiveData_WaitAuth(data);
		break;

	case ServiceSession::kSessionState_Authing:
		ReceiveData_Authing(data);
		break;

	default:
		assert(false);
		break;
	}
}

// 发送数据
void ServiceSession::SendData(const char * data, int32_t len)
{
	if (_state == kSessionState_Running)
	{
		assert(_socket != nullptr);
		// 直接发送
		_socket->Send(data, len);
	}
}

// 接收到数据
// 返回剩余多少数据
int32_t ServiceSession::OnReceived(char * data, int32_t len)
{
	assert(data && len > 0 && (_state == kSessionState_Running || _state == kSessionState_Authing || _state == kSessionState_WaitAuth));

	char * p = data;
	int32_t surplus = len;

	while (true)
	{
		uint16_t msg_size = 0;
		StreamReader msg_size_reader(p, surplus);
		if (!AutoDecode(msg_size_reader, msg_size) ||
			surplus - msg_size_reader.GetReadedLength() < msg_size)
		{
			break;
		}

		p += msg_size_reader.GetReadedLength();
		surplus -= msg_size_reader.GetReadedLength();

		std::shared_ptr<std::vector<char>> data = std::make_shared<std::vector<char>>(msg_size);
		if (msg_size > 0)
		{
			memcpy(&(*data)[0], p, msg_size);
			p += msg_size;
			surplus -= msg_size;
		}
			
		ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, kProxyServiceMsgId_SessionRecvData, _session_id, data);
	}

	return surplus;
}

// Socket关闭
// by_self: true表示主动请求的关闭操作
void ServiceSession::OnClosed(bool by_self, sframe::Error err)
{
	if (err)
	{
		LOG_INFO << "Connection with server(" << SocketAddrText(_socket->GetRemoteAddress()).Text() 
			<< ") closed with error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
	}
	else
	{
		LOG_INFO << "Connection with server(" << SocketAddrText(_socket->GetRemoteAddress()).Text() << ") closed" << ENDL;
	}

	std::shared_ptr<InsideServiceMessage<int32_t>> msg = std::make_shared<InsideServiceMessage<int32_t>>(_session_id);
	msg->dest_sid = 0;
	msg->src_sid = 0;
	msg->msg_id = kProxyServiceMsgId_SessionClosed;
	ServiceDispatcher::Instance().SendMsg(0, msg);
}

// 连接操作完成
void ServiceSession::OnConnected(sframe::Error err)
{
	bool success = true;

	if (err)
	{
		success = false;
		LOG_ERROR << "Connect to server(" << _remote_ip << ":" << _remote_port << ") error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
	}
	else
	{
		LOG_INFO << "Connect to server(" << _remote_ip << ":" << _remote_port << ") success" << ENDL;
	}

	// 通知连接完成
	ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, kProxyServiceMsgId_SessionConnectCompleted, _session_id, success);
	// 开始接收数据
	if (success)
	{
		_socket->StartRecv();
	}
}

// 运行状态接收数据处理
void ServiceSession::ReceiveData_Running(std::vector<char> & data)
{
	// 读取头部
	int32_t src_sid = 0;
	int32_t dest_sid = 0;
	uint16_t msg_id = 0;
	char * p = &data[0];
	uint32_t len = (uint32_t)data.size();
	StreamReader reader(p, len);
	if (AutoDecode(reader, src_sid, dest_sid, msg_id))
	{
		if (dest_sid > 0)
		{
			int32_t data_len = (int32_t)len - (int32_t)reader.GetReadedLength();
			assert(data_len >= 0);
			std::shared_ptr<NetServiceMessage> msg = std::make_shared<NetServiceMessage>();
			msg->dest_sid = dest_sid;
			msg->src_sid = src_sid;
			msg->msg_id = msg_id;
			msg->data = std::move(data);
			// 发送到目标服务
			ServiceDispatcher::Instance().SendMsg(dest_sid, msg);
		}
	}
}

// 验证状态接受数据处理
void ServiceSession::ReceiveData_Authing(std::vector<char> & data)
{
	uint8_t success;
	std::vector<int32_t> remote_service;

	StreamReader reader(&data[0], (uint32_t)data.size());
	if (!AutoDecode(reader, success, remote_service))
	{
		_reconnect = false;  // 不再进行自动重连
		_socket->Close();
		return;
	}

	if (!success)
	{
		_reconnect = false;  // 不再进行自动重连
		_socket->Close();
		return;
	}

	Start(remote_service);
}

// 等待验证状态接受数据处理
void ServiceSession::ReceiveData_WaitAuth(std::vector<char> & data)
{
	std::vector<int32_t> remote_service;
	int64_t send_time = 0;
	std::string sign;

	StreamReader reader(&data[0], (uint32_t)data.size());
	if (!AutoDecode(reader, remote_service, send_time, sign))
	{
		SendAuthCompletedMessage(false);
		_socket->Close();
		return;
	}

	// 验证签名
	std::string makesign;
	for (auto sid : remote_service)
	{
		makesign.append(std::to_string(sid));
	}
	makesign.append(std::to_string(send_time));
	if (!_proxy_service->GetLocalAuthKey().empty())
	{
		makesign.append(_proxy_service->GetLocalAuthKey());
	}

	MD5 md5(makesign);
	std::string my_sign = md5.GetResult();

	if (my_sign != sign)
	{
		SendAuthCompletedMessage(false);
		_socket->Close();
		return;
	}

	SendAuthCompletedMessage(true);
	Start(remote_service);
}

// 发送验证信息
bool ServiceSession::SendAuthMessage()
{
	auto & local_service = ServiceDispatcher::Instance().GetAllLocalSid();
	int64_t cur_time = TimeHelper::GetEpochSeconds();
	std::string makesign;
	for (auto sid : local_service)
	{
		makesign.append(std::to_string(sid));
	}
	makesign.append(std::to_string(cur_time));
	if (!_remote_key.empty())
	{
		makesign.append(_remote_key);
	}

	MD5 md5(makesign);
	std::string sign = md5.GetResult();

	// 发送验证消息
	uint16_t msg_size = 0;
	char buf[65536];

	StreamWriter writer(buf + sizeof(msg_size), 65536 - sizeof(msg_size));
	if (!AutoEncode(writer, local_service, cur_time, sign))
	{
		return false;
	}

	msg_size = (uint16_t)writer.GetStreamLength();
	StreamWriter msg_size_writer(buf, sizeof(msg_size));
	if (!AutoEncode(msg_size_writer, msg_size))
	{
		return false;
	}

	assert(_socket);
	_socket->Send(buf, msg_size + sizeof(msg_size));

	return true;
}

// 发送验证完成信息
bool ServiceSession::SendAuthCompletedMessage(bool success)
{
	uint8_t succ = success ? 1 : 0;
	std::vector<int32_t> local_service;

	if (success)
	{
		local_service = ServiceDispatcher::Instance().GetAllLocalSid();
	}

	uint16_t msg_size = 0;
	char buf[65536];

	StreamWriter writer(buf + sizeof(msg_size), 65536 - sizeof(msg_size));
	if (!AutoEncode(writer, succ, local_service))
	{
		return false;
	}

	msg_size = (uint16_t)writer.GetStreamLength();
	StreamWriter msg_size_writer(buf, sizeof(msg_size));
	if (!AutoEncode(msg_size_writer, msg_size))
	{
		return false;
	}

	assert(_socket);
	_socket->Send(buf, sizeof(msg_size) + msg_size);

	return true;
}

// 开始会话
void ServiceSession::Start(const std::vector<int32_t> & remote_service)
{
	_state = ServiceSession::kSessionState_Running;
	
	_proxy_service->StartSession(_session_id, remote_service);
}

// 开始连接定时器
void ServiceSession::StartConnectTimer(int32_t after_ms)
{
	_proxy_service->RegistSessionTimer(_session_id, after_ms, &ServiceSession::OnTimer_Connect);
}

// 定时：连接
int32_t ServiceSession::OnTimer_Connect(int64_t cur_ms)
{
	assert(_state == kSessionState_WaitConnect && _socket == nullptr && !_remote_ip.empty());

	LOG_INFO << "Start connect to server(" << _remote_ip << ":" << _remote_port << ")" << ENDL;

	_state = kSessionState_Connecting;
	_socket = sframe::TcpSocket::Create(ServiceDispatcher::Instance().GetIoService());
	_socket->SetMonitor(this);
	_socket->Connect(sframe::SocketAddr(_remote_ip.c_str(), _remote_port));

	// 只执行一次后停止
	return -1;
}