#ifndef __NETWORKENGINE_H__
#define __NETWORKENGINE_H__
#include <flecs.h>
#include <string>
#include <thread>
#include "RobotPal/Network/NetworkQueue.h"

#ifdef _WIN32
#include <Winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif defined(__EMSCRIPTEN__)
#include <emscripten/websocket.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting
};

struct NetworkConnectionState {
    ConnectionStatus status;
    char serverUrl[128];
    float ping; // 핑 상태 (옵션)
};


class NetworkEngine
{
public:
    NetworkEngine(flecs::world& world);
    ~NetworkEngine();

    void Connect(const std::string& url);
    void Disconnect();

    //파라미터는 바뀔 가능성 높음 일단
    void SendPacket(const std::vector<uint8_t>& rawData);
    std::optional<Packet> GetPacket();
    // [System] 프레임의 마지막(EndOfFrame)이나 별도 스레드에서 호출
    void FlushSendQueue();

    
private:
    void Start();
    void Stop();

    // --- [Recv Thread] ---
    void RecvLoop();

    // --- [Send Thread] --- 
    void SendLoop();

    //지금은 송신쪽만
    NetworkQueue m_SendQueue;

    //일단 나중에 구현
    NetworkQueue m_RecvQueue;
    flecs::world& m_World;

    bool isRunning;
    std::thread recvThread;
    std::thread sendThread;
};

struct NetworkEngineHandle
{
    const NetworkEngine* instance;
};


#endif