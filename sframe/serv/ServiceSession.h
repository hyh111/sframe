
#ifndef SFRAME_SERVICE_SESSION_H
#define SFRAME_SERVICE_SESSION_H

#include "../util/Serialization.h"
#include "../net/net.h"
#include "../util/Singleton.h"

namespace sframe {

class ProxyService;

// 服务会话（主要处理与网络中的服务的通信）
class ServiceSession : public TcpSocket::Monitor, public noncopyable
{
public:
	// 会话状态
	enum SessionState : int32_t
	{
		kSessionState_WaitConnect = 0,   // 等待连接
		kSessionState_Connecting,        // 正在连接
		kSessionState_Authing,           // 正在认证(主动发起连接方等待对方的验证结果)
		kSessionState_WaitAuth,          // 等待认证(被动连接方等待对方发送验证信息)
		kSessionState_Running,           // 运行中
	};

	static const int32_t kReconnectInterval = 5000;    // 重连间隔

public:
	ServiceSession(int32_t id, ProxyService * proxy_service, const std::string & remote_ip, uint16_t remote_port, const std::string & remote_key);

	ServiceSession(int32_t id, ProxyService * proxy_service, const std::shared_ptr<TcpSocket> & sock);

	~ServiceSession(){}

	void Init();

	// 关闭
	void Close();

	// 尝试释放
	bool TryFree();

	// 连接完成处理
	void DoConnectCompleted(bool success);

	// 接受数据
	void DoRecvData(std::vector<char> & data);

	// 发送数据
	void SendData(const char * data, int32_t len);

	// 获取状态
	SessionState GetState() const
	{
		return _state;
	}

	// 接收到数据
	// 返回剩余多少数据
	int32_t OnReceived(char * data, int32_t len) override;

	// Socket关闭
	// by_self: true表示主动请求的关闭操作
	void OnClosed(bool by_self, Error err) override;

	// 连接操作完成
	void OnConnected(Error err) override;

private:
	// 运行状态接收数据处理
	void ReceiveData_Running(std::vector<char> & data);

	// 验证状态接受数据处理
	void ReceiveData_Authing(std::vector<char> & data);

	// 等待验证状态接受数据处理
	void ReceiveData_WaitAuth(std::vector<char> & data);

	// 发送验证信息
	bool SendAuthMessage();

	// 发送验证完成信息
	bool SendAuthCompletedMessage(bool success);

	// 开始会话
	void Start(const std::vector<int32_t> & remote_service);

	// 开始连接定时器
	void StartConnectTimer(int32_t after_ms);

	// 定时：连接
	int32_t OnTimer_Connect(int64_t cur_ms);

private:
	ProxyService * _proxy_service;
	std::shared_ptr<TcpSocket> _socket;
	int32_t _session_id;
	SessionState _state;
	bool _reconnect;
	// 以下3个成员仅用于用于主动发起连接方
	std::string _remote_ip;
	uint16_t _remote_port;
	std::string _remote_key;
};

}

#endif
