
#ifndef SFRAME_SERVICE_SESSION_H
#define SFRAME_SERVICE_SESSION_H

#include <list>
#include "../util/Serialization.h"
#include "../net/net.h"
#include "../util/Singleton.h"
#include "../util/Timer.h"
#include "../util/Http.h"

namespace sframe {

class ProxyService;

// 服务会话（主要处理与网络中的服务的通信）
class ServiceSession : public TcpSocket::Monitor, public noncopyable, public SafeTimerRegistor<ServiceSession>
{
public:
	// 会话状态
	enum SessionState : int32_t
	{
		kSessionState_Initialize = 0,    // 初始状态
		kSessionState_WaitConnect,       // 等待连接
		kSessionState_Connecting,        // 正在连接
		kSessionState_Running,           // 运行中
	};

	static const int32_t kReconnectInterval = 3000;       // 自动重连间隔

public:
	ServiceSession(int32_t id, ProxyService * proxy_service, const std::string & remote_ip, uint16_t remote_port);

	ServiceSession(int32_t id, ProxyService * proxy_service, const std::shared_ptr<TcpSocket> & sock);

	virtual ~ServiceSession(){}

	void Init();

	// 关闭
	void Close();

	// 尝试释放
	bool TryFree();

	// 连接完成处理
	void DoConnectCompleted(bool success);

	// 发送数据
	void SendData(const std::shared_ptr<ProxyServiceMessage> & msg);

	// 发送数据
	void SendData(const char * data, size_t len);

	// 获取地址
	std::string GetRemoteAddrText() const;

	// 接收到数据
	// 返回剩余多少数据
	virtual int32_t OnReceived(char * data, int32_t len) override;

	// Socket关闭
	// by_self: true表示主动请求的关闭操作
	virtual void OnClosed(bool by_self, Error err) override;

	// 连接操作完成
	virtual void OnConnected(Error err) override;

	// 获取SessionId
	int32_t GetSessionId()
	{
		return _session_id;
	}

	// 获取状态
	SessionState GetState() const
	{
		return _state;
	}

private:

	// 开始连接定时器
	void SetConnectTimer(int32_t after_ms);

	// 定时：连接
	int32_t OnTimer_Connect();

	// 开始连接
	void StartConnect();

private:
	ProxyService * _proxy_service;
	std::shared_ptr<TcpSocket> _socket;
	int32_t _session_id;
	SessionState _state;
	TimerHandle _connect_timer;
	std::list<std::shared_ptr<ProxyServiceMessage>> _msg_cache;
	bool _reconnect;
	std::string _remote_ip;
	uint16_t _remote_port;
	std::shared_ptr<std::vector<char>> _cur_msg_data;
	size_t _cur_msg_size;
	size_t _cur_msg_readed_size;
};


// 管理会话
class AdminSession : public ServiceSession
{
public:

	AdminSession(int32_t id, ProxyService * proxy_service, const std::shared_ptr<TcpSocket> & sock)
		: ServiceSession(id, proxy_service, sock) {}

	virtual ~AdminSession() {}

	// 接收到数据
	// 返回剩余多少数据
	virtual int32_t OnReceived(char * data, int32_t len) override;

private:
	sframe::HttpRequestDecoder _http_decoder;
};

}

#endif
