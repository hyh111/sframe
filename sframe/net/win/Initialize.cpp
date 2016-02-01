
#include <WinSock2.h>
#include "Initialize.h"

WinSockInitial g_win_sock_initial;

WinSockInitial::WinSockInitial()
{
    WSAData wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        throw 0;
    }
}

WinSockInitial::~WinSockInitial()
{
    WSACleanup();
}