#pragma once
#include "RobotPal/Network/INetworkTransport.h"
#include <string>
#include <atomic>

#ifdef _WIN32
    #include <WinSock2.h>
    typedef SOCKET SocketHandle;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
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
    bool IsConnected() const override;

    // Engine에서 RecvLoop를 돌릴 때 사용
    int ReceiveRaw(uint8_t* buf, int maxSize);

private:
    SocketHandle m_Socket = INVALID_SOCKET;
    std::atomic<bool> m_IsConnected{false};
};
