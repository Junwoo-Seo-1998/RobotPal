#pragma once
#include <string>
#include <vector>
#include <deque> // 큐 사용
#include <iostream>

// 플랫폼별 헤더 ... (기존 코드 유지)
#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten/websocket.h>
#endif

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    bool Init(int port);
    void Update();
    void Send(const std::string& msg);
    bool IsConnected() const;

    // [변경] 메시지 큐 방식
    bool HasMessage();
    std::string PopMessage();

private:
    void Close();
    void SetNonBlocking(SOCKET s);

private:
    // [추가] 패킷 조립을 위한 버퍼와 큐
    std::string m_AccumulatedBuffer;
    std::deque<std::string> m_MessageQueue;

    bool m_IsConnected = false;

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBSOCKET_T m_WebSocket = 0;
#else
    SOCKET m_ListenSocket = INVALID_SOCKET;
    SOCKET m_ClientSocket = INVALID_SOCKET;
#endif
};