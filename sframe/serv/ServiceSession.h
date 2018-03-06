
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

// ·þÎñ»á»°£¨Ö÷Òª´¦ÀíÓëÍøÂçÖÐµÄ·þÎñµÄÍ¨ÐÅ£©
class ServiceSession : public TcpSocket::Monitor, public noncopyable, public SafeTimerRegistor<ServiceSession>
{
public:
	// »á»°×´Ì¬
	enum SessionState : int32_t
	{
		kSessionState_Initialize = 0,    // ³õÊ¼×´Ì¬
		kSessionState_WaitConnect,       // µÈ´ýÁ¬½Ó
		kSessionState_Connecting,        // ÕýÔÚÁ¬½Ó
		kSessionState_Running,           // ÔËÐÐÖÐ
	};

	static const int32_t kReconnectInterval = 3000;       // ×Ô¶¯ÖØÁ¬¼ä¸ô

public:
	ServiceSession(int32_t id, ProxyService * proxy_service, const std::string & remote_ip, uint16_t remote_port);

	ServiceSession(int32_t id, ProxyService * proxy_service, const std::shared_ptr<TcpSocket> & sock);

	virtual ~ServiceSession(){}

	void Init();

	// ¹Ø±Õ
	void Close();

	// ³¢ÊÔÊÍ·Å
	bool TryFree();

	// Á¬½ÓÍê³É´¦Àí
	void DoConnectCompleted(bool success);

	// ·¢ËÍÊý¾Ý
	void SendData(const std::shared_ptr<ProxyServiceMessage> & msg);

	// ·¢ËÍÊý¾Ý
	void SendData(const char * data, size_t len);

	// »ñÈ¡µØÖ·
	std::string GetRemoteAddrText() const;

	// ½ÓÊÕµ½Êý¾Ý
	// ·µ»ØÊ£Óà¶àÉÙÊý¾Ý
	virtual int32_t OnReceived(char * data, int32_t len) override;

	// Socket¹Ø±Õ
	// by_self: true±íÊ¾Ö÷¶¯ÇëÇóµÄ¹Ø±Õ²Ù×÷
	virtual void OnClosed(bool by_self, Error err) override;

	// Á¬½Ó²Ù×÷Íê³É
	virtual void OnConnected(Error err) override;

	// »ñÈ¡SessionId
	int32_t GetSessionId()
	{
		return _session_id;
	}

	// »ñÈ¡×´Ì¬
	SessionState GetState() const
	{
		return _state;
	}

private:

	// ¿ªÊ¼Á¬½Ó¶¨Ê±Æ÷
	void SetConnectTimer(int32_t after_ms);

	// ¶¨Ê±£ºÁ¬½Ó
	int32_t OnTimer_Connect();

	// ¿ªÊ¼Á¬½Ó
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


// ¹ÜÀí»á»°
class AdminSession : public ServiceSession
{
public:

	AdminSession(int32_t id, ProxyService * proxy_service, const std::shared_ptr<TcpSocket> & sock)
		: ServiceSession(id, proxy_service, sock) {}

	virtual ~AdminSession() {}

	// ½ÓÊÕµ½Êý¾Ý
	// ·µ»ØÊ£Óà¶àÉÙÊý¾Ý
	virtual int32_t OnReceived(char * data, int32_t len) override;

private:
	sframe::HttpRequestDecoder _http_decoder;
};

}

#endif
