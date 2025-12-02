#include "RobotPal/Network/INetworkTransport.h"
#include <string>
#include <vector>
#include <cstring>
#include <atomic>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketHandle;
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int SocketHandle;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSE_SOCKET(s) close(s)
#endif

#include <iostream>

// 간단 구현: URL은 "host:port"
class TcpNetworkTransport : public INetworkTransport
{
public:
    TcpNetworkTransport()
    {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    }

    virtual ~TcpNetworkTransport()
    {
        Disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool Connect(const std::string& url) override
    {
        // parse host:port
        auto pos = url.find(':');
        if (pos == std::string::npos) return false;
        std::string host = url.substr(0, pos);
        std::string port = url.substr(pos + 1);

        struct addrinfo hints;
        struct addrinfo* res = nullptr;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int rv = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
        if (rv != 0 || res == nullptr) {
            return false;
        }

        SocketHandle sock = INVALID_SOCKET;
        struct addrinfo* p;
        for (p = res; p != nullptr; p = p->ai_next) {
            sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock == INVALID_SOCKET) continue;
            if (::connect(sock, p->ai_addr, p->ai_addrlen) == 0) {
                break;
            }
            CLOSE_SOCKET(sock);
            sock = INVALID_SOCKET;
        }

        freeaddrinfo(res);

        if (sock == INVALID_SOCKET) {
            return false;
        }

        m_Socket = sock;
        m_IsConnected = true;
        return true;
    }

    void Disconnect() override
    {
        if (m_Socket != INVALID_SOCKET) {
            CLOSE_SOCKET(m_Socket);
            m_Socket = INVALID_SOCKET;
        }
        m_IsConnected = false;
    }

    void Send(const std::vector<uint8_t>& data) override
    {
        if (!m_IsConnected || m_Socket == INVALID_SOCKET) return;

        const uint8_t* ptr = data.data();
        size_t toSend = data.size();
        while (toSend > 0) {
            int sent = ::send(m_Socket, reinterpret_cast<const char*>(ptr), static_cast<int>(toSend), 0);
            if (sent == SOCKET_ERROR) {
                // 실패 시 연결 끊김 처리
                m_IsConnected = false;
                return;
            }
            ptr += sent;
            toSend -= sent;
        }
    }

    bool IsConnected() const override
    {
        return m_IsConnected;
    }


private:
    SocketHandle m_Socket = INVALID_SOCKET;
    std::atomic<bool> m_IsConnected{false};
};
