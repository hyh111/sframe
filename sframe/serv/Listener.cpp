
#include "Listener.h"
#include "ServiceDispatcher.h"
#include "../util/Log.h"

using namespace sframe;

void ServiceTcpConnHandler::HandleTcpConn(const std::shared_ptr<TcpSocket> & sock, const ListenAddress & listen_addr)
{
	if (_handle_services.empty())
	{
		return;
	}

	int32_t hand_sid = -1;

	if (_handle_services.size() == 1)
	{
		hand_sid = *(_handle_services.begin());
	}
	else
	{
		hand_sid = *_it_cur_sid;
		_it_cur_sid++;
		if (_it_cur_sid == _handle_services.end())
		{
			_it_cur_sid = _handle_services.begin();
		}
	}
	
	assert(hand_sid >= 0);

	std::shared_ptr<NewConnectionMessage> new_conn_msg = std::make_shared<NewConnectionMessage>(sock, listen_addr);
	sframe::ServiceDispatcher::Instance().SendMsg(hand_sid, new_conn_msg);
}



Listener::Listener(const std::string & ip, uint16_t port, const std::string & desc_name, std::shared_ptr<TcpConnHandler> conn_handler)
{
	_running = false;
	_addr.ip = ip;
	_addr.port = port;
	_addr.desc_name = desc_name;
	_conn_handler = conn_handler;
}

bool Listener::Start()
{
	if (_addr.ip.empty())
	{
		return false;
	}

	_acceptor = sframe::TcpAcceptor::Create(ServiceDispatcher::Instance().GetIoService());
	_acceptor->SetMonitor(this);
	sframe::Error err = _acceptor->Start(sframe::SocketAddr(_addr.ip.c_str(), _addr.port));
	if (err)
	{
		_acceptor.reset();
		LOG_ERROR << "Listen " << _addr.ip << ':' << _addr.port << "(" << _addr.desc_name << ") error|" << err.Code() << "|" << sframe::ErrorMessage(err).Message() << std::endl;
		return false;
	}

	LOG_INFO << "Listener " << _addr.ip << ':' << _addr.port << "(" << _addr.desc_name << ") started" << std::endl;
	_running = true;

	return true;
}

void Listener::Stop()
{
	if (_running)
	{
		assert(_acceptor);
		_acceptor->Close();
	}
}

// 连接通知
void Listener::OnAccept(std::shared_ptr<TcpSocket> socket, Error err)
{
	if (err)
	{
		LOG_ERROR << "Listener " << _addr.ip << ':' << _addr.port << "(" << _addr.desc_name << ") accept connection error|" << err.Code() << "|" << sframe::ErrorMessage(err).Message() << std::endl;
		return;
	}

	if (!_conn_handler)
	{
		LOG_ERROR << "Listener " << _addr.ip << ':' << _addr.port << "(" << _addr.desc_name << ") have no connection handler" << std::endl;
		return;
	}

	_conn_handler->HandleTcpConn(socket, _addr);
}

// 停止
void Listener::OnClosed(Error err)
{
	if (err)
	{
		LOG_ERROR << "Listener " << _addr.ip << ':' << _addr.port << "(" << _addr.desc_name << ") stoped with error|" 
			<< err.Code() << "|" << sframe::ErrorMessage(err).Message() << std::endl;
	}
	else
	{
		LOG_INFO << "Listener " << _addr.ip << ':' << _addr.port << "(" << _addr.desc_name << ") stoped" << std::endl;
	}

	_running = false;
}