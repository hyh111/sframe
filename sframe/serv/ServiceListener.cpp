
#include "ServiceListener.h"
#include "ServiceDispatcher.h"
#include "ProxyService.h"
#include "../util/Log.h"
#include "../util/TimeHelper.h"

using namespace sframe;
using namespace sframe;

// 开始
bool ServiceListener::Start()
{
	if (_listen_ip.empty())
	{
		return false;
	}

	_acceptor = sframe::TcpAcceptor::Create(ServiceDispatcher::Instance().GetIoService());
	_acceptor->SetMonitor(this);
	sframe::Error err = _acceptor->Start(sframe::SocketAddr(_listen_ip.c_str(), _listen_port));
	if (err)
	{
		_acceptor.reset();
		LOG_ERROR << "Start ServiceListener Error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
		return false;
	}

	return true;
}

// 停止
void ServiceListener::Stop()
{
	if (_acceptor != nullptr)
	{
		if (!_acceptor->Close())
		{
			std::shared_ptr<InsideServiceMessage<>> msg = std::make_shared<InsideServiceMessage<>>();
			msg->dest_sid = 0;
			msg->src_sid = 0;
			msg->msg_id = kProxyServiceMsgId_ServiceListenerClosed;
			ServiceDispatcher::Instance().SendMsg(0, msg);
		}
	}
}

// 连接通知
void ServiceListener::OnAccept(std::shared_ptr<sframe::TcpSocket> socket, sframe::Error err)
{
	if (err) 
	{
		LOG_ERROR << "ServiceListener accept connection error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
		return;
	}

	std::shared_ptr<InsideServiceMessage<std::shared_ptr<sframe::TcpSocket>>> msg =
		std::make_shared<InsideServiceMessage<std::shared_ptr<sframe::TcpSocket>>>(socket);
	msg->dest_sid = 0;
	msg->src_sid = 0;
	msg->msg_id = kProxyServiceMsgId_AddNewSession;

	ServiceDispatcher::Instance().SendMsg(0, msg);
}

// 停止
void ServiceListener::OnClosed(sframe::Error err)
{
	if (err)
	{
		LOG_INFO << "ServiceListener stoped with Error(" << err.Code() << "): " << sframe::ErrorMessage(err).Message() << ENDL;
	}
	else
	{
		LOG_INFO << "ServiceListener stoped with no error" << ENDL;
	}

	std::shared_ptr<InsideServiceMessage<>> msg = std::make_shared<InsideServiceMessage<>>();
	msg->dest_sid = 0;
	msg->src_sid = 0;
	msg->msg_id = kProxyServiceMsgId_ServiceListenerClosed;
	ServiceDispatcher::Instance().SendMsg(0, msg);
}
