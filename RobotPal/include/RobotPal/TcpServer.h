#pragma once

#include <string>
#include <vector>
#include <iostream>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

class TcpServer{
public:
    TcpServer();
    ~TcpServer();

    bool Start(int port);
    void Update();
    void Send(const std::string& msg);
    bool isConnected()const{ return m_ClientSocket != INVALID_SOCKET;}
    std::string GetLastReceivedData();
private:
    void Close();
    void SetNonBlocking(SOCKET s);
private:
    SOCKET m_ListenSocket = INVALID_SOCKET; // 문지기 소켓
    SOCKET m_ClientSocket = INVALID_SOCKET; // 손님 소켓
    
    std::string m_ReceivedBuffer; // 수신된 데이터 저장소
};