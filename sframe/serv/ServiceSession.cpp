
#include "ServiceDispatcher.h"
#include "ServiceSession.h"
#include "ProxyService.h"
#include "../util/Log.h"
#include "../util/TimeHelper.h"
#include "../util/md5.h"

using namespace sframe;

ServiceSession::ServiceSession(int32_t id, ProxyService * proxy_service, const std::string & remote_ip, uint16_t remote_port)
	: _session_id(id), _remote_ip(remote_ip), _remote_port(remote_port), _proxy_service(proxy_service), 
	_state(kSessionState_Initialize),  _reconnect(true), _cur_msg_size(0), _cur_msg_readed_size(0)
{
	assert(!remote_ip.empty() && proxy_service);
}

ServiceSession::ServiceSession(int32_t id, ProxyService * proxy_service, const std::shared_ptr<sframe::TcpSocket> & sock)
	: _session_id(id), _socket(sock), _state(kSessionState_Running), _proxy_service(proxy_service), 
	_reconnect(false), _cur_msg_size(0), _cur_msg_readed_size(0)
{
	assert(sock != nullptr && proxy_service);
}


void ServiceSession::Init()
{
	if (!_socket)
	{
		SetConnectTimer(0);
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
		bool by_self = true;
		std::shared_ptr<InsideServiceMessage<bool, int32_t>> msg = std::make_shared<InsideServiceMessage<bool, int32_t>>(by_self, _session_id);
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

	_state = kSessionState_WaitConnect;
	_socket.reset();
	// 开启连接定时器
	SetConnectTimer(kReconnectInterval);

	return false;
}

// 连接操作完成处理
void ServiceSession::DoConnectCompleted(bool success)
{
	if (!success)
	{
		// 清空缓存的发送数据
		_msg_cache.clear();
		// 开启连接定时器
		_socket.reset();
		_state = kSessionState_WaitConnect;
		SetConnectTimer(kReconnectInterval);
		return;
	}

	// 开始会话
	_state = ServiceSession::kSessionState_Running;
	assert(_socket->IsOpen());

	// 之前缓存的数据立即发送出去
	for (auto & msg : _msg_cache)
	{
		std::string data;
		if (msg->Serialize(data))
		{
			_socket->Send(data.data(), (int32_t)data.size());
		}
		else
		{
			LOG_ERROR << "Serialize mesage error" << std::endl;
		}
	}
	_msg_cache.clear();
}

// 发送数据
void ServiceSession::SendData(const std::shared_ptr<ProxyServiceMessage> & msg)
{
	if (_state != ServiceSession::kSessionState_Running)
	{
		// 缓存下来
		_msg_cache.push_back(msg);
		// 如果当前处于等待连接状态，删除timer，立即开始连接
		if (_state == ServiceSession::kSessionState_WaitConnect)
		{
			assert(Timer::IsTimerAlive(_connect_timer));
			GetTimerManager()->DeleteTimer(_connect_timer);
			StartConnect();
		}
	}
	else
	{
		assert(_socket);
		// 直接发送
		std::string data;
		if (msg->Serialize(data))
		{
			_socket->Send(data.c_str(), (int32_t)data.size());
		}
	}
}

// 发送数据
void ServiceSession::SendData(const char * data, size_t len)
{
	if (_state == ServiceSession::kSessionState_Running && data && len > 0)
	{
		assert(_socket);
		_socket->Send(data, (int32_t)len);
	}
}

// 获取地址
std::string ServiceSession::GetRemoteAddrText() const
{
	if (!_socket)
	{
		return std::string();
	}

	return SocketAddrText(_socket->GetRemoteAddress()).Text();
}

// 接收到数据
// 返回剩余多少数据
int32_t ServiceSession::OnReceived(char * data, int32_t len)
{
	assert(data && len > 0 && _state == kSessionState_Running);

	char * p = data;
	size_t surplus = (int32_t)len;

	while (surplus > 0)
	{
		if (_cur_msg_size > 0)
		{
			assert(_cur_msg_data && _cur_msg_size > _cur_msg_readed_size);

			size_t cur_msg_remain_size = _cur_msg_size - _cur_msg_readed_size;
			size_t read_size = std::min(surplus, cur_msg_remain_size);

			memcpy(_cur_msg_data->data() + _cur_msg_readed_size, p, read_size);
			p += read_size;
			surplus -= read_size;
			_cur_msg_readed_size += read_size;

			if (_cur_msg_readed_size >= _cur_msg_size)
			{
				ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, 0, kProxyServiceMsgId_SessionRecvData, _session_id, _cur_msg_data);
				_cur_msg_data.reset();
				_cur_msg_size = 0;
				_cur_msg_readed_size = 0;
			}
		}
		else
		{
			assert(!_cur_msg_data && _cur_msg_readed_size == 0);

			StreamReader msg_size_reader(p, surplus);
			if (!msg_size_reader.ReadSizeField(_cur_msg_size))
			{
				_cur_msg_size = 0;
				break;
			}

			assert(msg_size_reader.GetReadedLength() <= surplus);
			p += msg_size_reader.GetReadedLength();
			surplus -= msg_size_reader.GetReadedLength();

			_cur_msg_data = std::make_shared<std::vector<char>>(_cur_msg_size);

			if (_cur_msg_size == 0)
			{
				ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, 0, kProxyServiceMsgId_SessionRecvData, _session_id, _cur_msg_data);
				_cur_msg_data.reset();
			}
		}
	}

	return (int32_t)surplus;
}

// Socket关闭
// by_self: true表示主动请求的关闭操作
void ServiceSession::OnClosed(bool by_self, sframe::Error err)
{
	if (err)
	{
		LOG_INFO << "Connection with " << SocketAddrText(_socket->GetRemoteAddress()).Text() 
			<< " closed with error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
	}
	else
	{
		LOG_INFO << "Connection with " << SocketAddrText(_socket->GetRemoteAddress()).Text() << " closed" << ENDL;
	}

	std::shared_ptr<InsideServiceMessage<bool, int32_t>> msg = std::make_shared<InsideServiceMessage<bool, int32_t>>(by_self, _session_id);
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
	ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, 0, kProxyServiceMsgId_SessionConnectCompleted, _session_id, success);
	// 开始接收数据
	if (success)
	{
		_socket->StartRecv();
	}
}

// 开始连接定时器
void ServiceSession::SetConnectTimer(int32_t after_ms)
{
	_connect_timer = RegistTimer(after_ms, &ServiceSession::OnTimer_Connect);
}

// 定时：连接
int32_t ServiceSession::OnTimer_Connect()
{
	StartConnect();
	// 只执行一次后停止
	return -1;
}

// 开始连接
void ServiceSession::StartConnect()
{
	assert((_state == kSessionState_Initialize || _state == kSessionState_WaitConnect) && !_socket && !_remote_ip.empty());
	LOG_INFO << "Start connect to server(" << _remote_ip << ":" << _remote_port << ")" << ENDL;
	_state = kSessionState_Connecting;
	_socket = sframe::TcpSocket::Create(ServiceDispatcher::Instance().GetIoService());
	_socket->SetMonitor(this);
	_socket->SetTcpNodelay(true);
	_socket->Connect(sframe::SocketAddr(_remote_ip.c_str(), _remote_port));
}



// 接收到数据
// 返回剩余多少数据
int32_t AdminSession::OnReceived(char * data, int32_t len)
{
	assert(data && len > 0 && GetState() == kSessionState_Running);

	int32_t session_id = GetSessionId();

	std::string err_msg;
	size_t readed = _http_decoder.Decode(data, len, err_msg);
	if (!err_msg.empty())
	{
		LOG_ERROR << "AdminSession(" << session_id << ") decode http request error|" << err_msg << std::endl;
		// 关闭连接
		return -1;
	}

	if (_http_decoder.IsDecodeCompleted())
	{
		std::shared_ptr<sframe::HttpRequest> http_req = _http_decoder.GetResult();
		_http_decoder.Reset();
		assert(http_req);
		ServiceDispatcher::Instance().SendInsideServiceMsg(0, 0, 0, kProxyServiceMsgId_AdminCommand, session_id, http_req);
	}

	return len - (int32_t)readed;
}