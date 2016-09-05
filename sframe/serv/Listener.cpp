
#include "Listener.h"
#include "ServiceDispatcher.h"
#include "../util/Log.h"

using namespace sframe;

int32_t ConnDistributeStrategy::DistributeHandleService(const std::shared_ptr<TcpSocket> & sock)
{
	if (_handle_services.empty())
	{
		assert(false);
		return -1;
	}

	if (_handle_services.size() == 1)
	{
		return *(_handle_services.begin());
	}

	int32_t sid = *_it_cur_sid;
	_it_cur_sid++;
	if (_it_cur_sid == _handle_services.end())
	{
		_it_cur_sid = _handle_services.begin();
	}

	return sid;
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
	_running.store(true);

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

	int32_t handle_sid = _distribute_strategy->DistributeHandleService(socket);
	if (handle_sid < 0)
	{
		assert(false);
		return;
	}

	std::shared_ptr<NewConnectionMessage> new_conn_msg = std::make_shared<NewConnectionMessage>(socket, _addr);
	GetServiceDispatcher().SendMsg(handle_sid, new_conn_msg);
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

	_running.store(false);
}