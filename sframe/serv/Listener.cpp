
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
	if (_ip.empty())
	{
		return false;
	}

	_acceptor = sframe::TcpAcceptor::Create(ServiceDispatcher::Instance().GetIoService());
	_acceptor->SetMonitor(this);
	sframe::Error err = _acceptor->Start(sframe::SocketAddr(_ip.c_str(), _port));
	if (err)
	{
		_acceptor.reset();
		LOG_ERROR << "Listen " << _ip << ':' << _port << " error(" << err.Code() << "), " << sframe::ErrorMessage(err).Message() << ENDL;
		return false;
	}

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
		LOG_ERROR << "Listener(" << _ip << ':' << _port << ")accept connection error(" << err.Code() << "), " << sframe::ErrorMessage(err).Message() << ENDL;
		return;
	}

	int32_t handle_sid = _distribute_strategy->DistributeHandleService(socket);
	if (handle_sid < 0)
	{
		assert(false);
		return;
	}

	std::shared_ptr<NewConnectionMessage> new_conn_msg = std::make_shared<NewConnectionMessage>(socket);
	GetServiceDispatcher().SendMsg(handle_sid, new_conn_msg);
}

// 停止
void Listener::OnClosed(Error err)
{
	if (err)
	{
		LOG_INFO << "Listener(" << _ip << ':' << _port << ")stoped with Error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
	}
	else
	{
		LOG_INFO << "Listener(" << _ip << ':' << _port << ")stoped with no error" << ENDL;
	}

	_running.store(false);
}