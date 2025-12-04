#include "RobotPal/Network/TcpNetworkTransport.h"
#include <iostream>

#ifndef __EMSCRIPTEN__

TcpNetworkTransport::TcpNetworkTransport()
{
#ifdef _WIN32
    WSADATA d;
    WSAStartup(MAKEWORD(2, 2), &d);
#endif
}

TcpNetworkTransport::~TcpNetworkTransport()
{
    Disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool TcpNetworkTransport::Connect(const std::string& url)
{
    if (m_IsConnected) return true;

    // URL 파싱 (간단한 버전: "ip:port" 형태 가정, 없으면 포트 80)
    std::string host = url;
    std::string port = "80";
    auto pos = url.find(':');
    if (pos != std::string::npos) {
        host = url.substr(0, pos);
        port = url.substr(pos + 1);
    }

    struct addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0) {
        std::cerr << "[TCP] DNS lookup failed\n";
        return false;
    }

    SocketHandle sock = INVALID_SOCKET;
    for (auto p = res; p != nullptr; p = p->ai_next) {
#ifdef _WIN32
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#else
        sock = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
#endif
        if (sock == INVALID_SOCKET) continue;

        if (connect(sock, p->ai_addr, (int)p->ai_addrlen) == 0) break;

#ifdef _WIN32
        closesocket(sock);
#else
        ::close(sock);
#endif
        sock = INVALID_SOCKET;
    }
    freeaddrinfo(res);

    if (sock == INVALID_SOCKET) {
        std::cerr << "[TCP] Connection failed\n";
        return false;
    }

    m_Socket = sock;
    m_IsConnected = true;
    return true;
}

void TcpNetworkTransport::Disconnect()
{
    if (m_Socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_Socket);
#else
        ::close(m_Socket);
#endif
        m_Socket = INVALID_SOCKET;
    }
    m_IsConnected = false;
}

void TcpNetworkTransport::Send(const std::vector<uint8_t>& data)
{
    if (!m_IsConnected || data.empty()) return;

    const char* ptr = (const char*)data.data();
    size_t left = data.size();

    while (left > 0) {
#ifdef _WIN32
        int sent = send(m_Socket, ptr, (int)left, 0);
#else
        ssize_t sent = ::send(m_Socket, ptr, left, 0);
#endif
        if (sent <= 0) {
            std::cerr << "[TCP] Send failed, disconnecting...\n";
            Disconnect();
            return;
        }
        ptr += sent;
        left -= sent;
    }
}
#endif