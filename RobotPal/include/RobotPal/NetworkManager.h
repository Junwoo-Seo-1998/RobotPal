/**
 * @file NetworkManager.h
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include <string>
#include <vector>
#include <iostream>

// [플랫폼별 헤더 및 타입 정의]
#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    // Linux / Emscripten (Web)
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netinet/in.h>
    
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

// [웹 빌드 여부 확인]
#ifdef __EMSCRIPTEN__
    #include <emscripten/websocket.h>
#endif

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    // 초기화 (PC는 Server, Web은 Client로 동작)
    bool Init(int port);

    // 매 프레임 업데이트 (비동기 처리)
    void Update();

    // 메시지 전송
    void Send(const std::string& msg);

    // 연결 여부 확인
    bool IsConnected() const;

    // 수신 데이터 가져오기
    std::string GetLastReceivedData();

private:
    void Close();
    void SetNonBlocking(SOCKET s);

private:
    // 공통 데이터
    std::string m_ReceivedBuffer;
    bool m_IsConnected = false;

#ifdef __EMSCRIPTEN__
    // [웹 전용] WebSocket 핸들
    EMSCRIPTEN_WEBSOCKET_T m_WebSocket = 0;
#else
    // [PC 전용] TCP 소켓
    SOCKET m_ListenSocket = INVALID_SOCKET;
    SOCKET m_ClientSocket = INVALID_SOCKET;
#endif
};