#include "RobotPal/Streaming/TCPStreamingManager.h"

#ifndef __EMSCRIPTEN__

TCPStreamingManager::TCPStreamingManager() : m_sock(-1) {}
TCPStreamingManager::~TCPStreamingManager() { Shutdown(); }

void TCPStreamingManager::Init() {
#ifdef _WIN32
    WSADATA d;
    if (WSAStartup(MAKEWORD(2,2), &d) != 0)
        std::cerr << "WSAStartup failed\n";
#endif
    m_StatusMessage = "Ready";
    m_IsConnected = false;
}

void TCPStreamingManager::Shutdown() {
    Disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

void TCPStreamingManager::ConnectToServer(const std::string& url) {
    if (m_sock != -1) return;

    std::string host = url;
    std::string port = "80";

    auto pos = url.find(':');
    if (pos != std::string::npos) {
        host = url.substr(0, pos);
        port = url.substr(pos + 1);
    }

    struct addrinfo hints{};
    struct addrinfo *res = nullptr;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int r = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if (r != 0 || !res) {
        m_StatusMessage = "DNS failed";
        return;
    }

    int sockfd = -1;

    for (auto p = res; p != nullptr; p = p->ai_next) {
#ifdef _WIN32
        sockfd = (int)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == INVALID_SOCKET) continue;
#else
        sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;
#endif

        if (connect(sockfd, p->ai_addr, (int)p->ai_addrlen) == 0)
            break;

#ifdef _WIN32
        closesocket(sockfd);
#else
        ::close(sockfd);
#endif
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd == -1) {
        m_StatusMessage = "TCP connect failed";
        return;
    }

    m_sock = sockfd;
    m_IsConnected = true;
    m_StatusMessage = "Connected (TCP)";
}

void TCPStreamingManager::Disconnect() {
    if (m_sock != -1) {
#ifdef _WIN32
        closesocket(m_sock);
#else
        ::close(m_sock);
#endif
        m_sock = -1;
    }
    m_IsConnected = false;
    m_StatusMessage = "Disconnected";
}

void TCPStreamingManager::SendFrame(const FrameData& frame) {
    if (!m_IsConnected || frame.pixel_data.empty()) return;

    // JPEG 압축
    WriteContext ctx;
    ctx.buffer.reserve(frame.width * frame.height);

    const int quality = 85;
    int ok = stbi_write_jpg_to_func(
        write_func,
        &ctx,
        frame.width,
        frame.height,
        frame.channels,
        frame.pixel_data.data(),
        quality
    );

    if (!ok || ctx.buffer.empty()) {
        std::cerr << "JPEG compress failed\n";
        return;
    }

    // 길이 prefix
    uint32_t size_be = htonl((uint32_t)ctx.buffer.size());
    const uint8_t* p = (const uint8_t*)&size_be;
    size_t left = 4;

    while (left > 0) {
#ifdef _WIN32
        int sent = send(m_sock, (const char*)p, (int)left, 0);
#else
        ssize_t sent = ::send(m_sock, p, left, 0);
#endif
        if (sent <= 0) {
            Disconnect();
            return;
        }
        left -= sent;
        p += sent;
    }

    // JPEG 데이터 전송
    p = ctx.buffer.data();
    left = ctx.buffer.size();

    while (left > 0) {
#ifdef _WIN32
        int sent = send(m_sock, (const char*)p, (int)left, 0);
#else
        ssize_t sent = ::send(m_sock, p, left, 0);
#endif
        if (sent <= 0) {
            Disconnect();
            return;
        }
        left -= sent;
        p += sent;
    }
}

void TCPStreamingManager::write_func(void* ctx, void* data, int size) {
    WriteContext* c = static_cast<WriteContext*>(ctx);
    c->buffer.insert(c->buffer.end(), (uint8_t*)data, (uint8_t*)data + size);
}

#endif // !__EMSCRIPTEN__
