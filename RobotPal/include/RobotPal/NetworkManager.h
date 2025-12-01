#pragma once
#include <string>
#include <vector>
#include <deque> // 큐 사용
#include <iostream>

// [플랫폼별 헤더]
#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#elif defined(__EMSCRIPTEN__)
    #include <emscripten/websocket.h>
#else
    // Linux / Mac
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    // [변경] 서버 주소(IP)와 포트를 받아 접속 (Client Mode)
    bool Init(const std::string& ip, int port);
    
    // [변경] SendQueue 비우기(전송) + RecvQueue 채우기(수신)
    void Update();
    
    // [변경] 바로 보내지 않고 SendQueue에 넣음
    void Send(const std::string& msg);
    
    bool IsConnected() const;

    // [수신 큐 관련]
    bool HasMessage();
    std::string PopMessage();

    // [Web 전용] 콜백에서 RecvQueue에 넣기 위함
    void PushToRecvQueue(const std::string& packet);

private:
    void Close();
    void SetNonBlocking(int s);

private:
    // [핵심] 입출력 큐 2개 분리
    std::deque<std::string> m_SendQueue; // 보낼 데이터 (Write Queue)
    std::deque<std::string> m_RecvQueue; // 받은 데이터 (Read Queue)

    // 패킷 조립용 버퍼
    std::string m_AccumulatedBuffer;

    bool m_IsConnected = false;

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBSOCKET_T m_WebSocket = 0;
#else
    int m_Socket = -1; // 클라이언트 소켓 하나만 있으면 됨
#endif
};