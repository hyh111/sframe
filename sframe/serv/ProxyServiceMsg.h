
#ifndef SFRAME_PROXY_SERVICE_MSG_H
#define SFRAME_PROXY_SERVICE_MSG_H

namespace sframe {

// 代理服务消息号
enum ProxyServiceMsgId
{
	kProxyServiceMsgId_SessionClosed = 1,
	kProxyServiceMsgId_SessionConnectCompleted,
	kProxyServiceMsgId_SessionRecvData,
	kProxyServiceMsgId_AdminCommand,
	kProxyServiceMsgId_SendAdminCommandResponse,
};

}

#endif