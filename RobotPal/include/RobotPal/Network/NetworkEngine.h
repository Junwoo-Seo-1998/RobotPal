#ifndef __NETWORKENGINE_H__
#define __NETWORKENGINE_H__

#include <flecs.h>
#include <string>
#include <thread>
#include <atomic>

#include "RobotPal/Network/NetworkQueue.h"
#include "RobotPal/Network/NetworkTransport.h"

enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting
};

struct NetworkConnectionState {
    ConnectionStatus status;
    char serverUrl[128];
    float ping;
};

class NetworkEngine
{
public:
    explicit NetworkEngine(flecs::world& world);
    ~NetworkEngine();

    // --- Connection ---
    bool TryConnect(const std::string& url);
    void Disconnect();
    bool IsConnected() const;

    // --- TX/RX ---
    void SendPacket(const std::vector<uint8_t>& rawData);
    std::optional<Packet> GetPacket();      
    void FlushSendQueue();                  

private:
    // --- Internal Thread Control ---
    void Start();
    void Stop();
    void RecvLoop();
    void SendLoop();

private:
    flecs::world& m_World;

    std::shared_ptr<NetworkTransport> m_Transport;

    NetworkQueue m_SendQueue;
    NetworkQueue m_RecvQueue;

    std::atomic<bool> isRunning;
    std::thread recvThread;
    std::thread sendThread;

    std::string m_CurrentUrl;
};

struct NetworkEngineHandle
{    
    NetworkEngine* instance=nullptr;   
};

#endif
