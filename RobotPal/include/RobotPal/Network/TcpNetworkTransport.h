#pragma once
#include "RobotPal/Network/NetworkTransport.h"
#include "RobotPal/Network/StreamingSendPolicy.h"
#include <thread>
#include <atomic>
#include <vector>
#include <string>

#ifndef __EMSCRIPTEN__

// 플랫폼별 소켓 헤더 처리
#ifdef _WIN32
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketHandle;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <fcntl.h>
    typedef int SocketHandle;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

class TcpNetworkTransport : public NetworkTransport
{
public:
    TcpNetworkTransport();
    virtual ~TcpNetworkTransport();

    // [인터페이스 구현]
    bool Connect(const std::string& url) override;
    void Disconnect() override;
    // Outgoing stream is replaceable video; do not share with reliable commands.
    void Send(const std::vector<uint8_t>& data) override;
    bool IsConnected() const override { return m_IsConnected; }
    size_t PendingPackets() const { return m_SendQueue.Size(); }
    size_t PendingBytes() const { return m_SendQueue.Bytes(); }
    size_t DroppedPackets() const { return m_SendQueue.Dropped(); }

private:
    // 내부 스레드 함수
    void RecvWorker();
    void SendWorker();

    // 소켓 헬퍼
    void StopWorkers();
    void CleanupSocket();

private:
    SocketHandle m_Socket = INVALID_SOCKET;
    std::atomic<bool> m_IsConnected { false };
    std::atomic<bool> m_IsRunning { false }; 

    // 스레드 및 큐
    std::thread m_RecvThread;
    std::thread m_SendThread;

    std::mutex m_enqueueMutex;
    robotpal::BoundedSendQueue m_SendQueue;
};

#endif