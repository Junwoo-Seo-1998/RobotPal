#pragma once
#include "RobotPal/Network/INetworkTransport.h"

#ifndef __EMSCRIPTEN__

#ifdef _WIN32
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET SocketHandle;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int SocketHandle;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

class TcpNetworkTransport : public INetworkTransport
{
public:
    TcpNetworkTransport();
    virtual ~TcpNetworkTransport();

    bool Connect(const std::string& url) override;
    void Disconnect() override;
    void Send(const std::vector<uint8_t>& data) override;
    bool IsConnected() const override { return m_IsConnected; }

private:
    SocketHandle m_Socket = INVALID_SOCKET;
    bool m_IsConnected = false;
};

#endif // !__EMSCRIPTEN__