#include "RobotPal/Network/NetworkEngine.h"
#include "RobotPal/Network/TransportFactory.h"

#ifndef _WIN32
#include <unistd.h>
#endif

NetworkEngine::NetworkEngine(flecs::world &world)
    : m_World(world), m_SendQueue(), m_RecvQueue(), isRunning(false), recvThread(), sendThread()
{
    m_Transport = TransportFactory::Create();


    m_World.set<NetworkEngineHandle>({*this});
    m_World.set<NetworkConnectionState>({ConnectionStatus::Disconnected, "", 0.0f});
}

NetworkEngine::~NetworkEngine()
{
    Disconnect();
}

void NetworkEngine::Connect(const std::string &url) 
{
    if(isRunning) return;

    if (m_Transport && m_Transport->Connect(url)) {
        Start(); // 제어 스레드 시작
    }
}
void NetworkEngine::Disconnect() 
{
    Stop();
    if (m_Transport) m_Transport->Disconnect();
}
bool NetworkEngine::IsConnected() const {
    return m_Transport && m_Transport->IsConnected();
}

void NetworkSleep(int milliseconds)
{
#ifdef _WIN32
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void NetworkEngine::SendPacket(const std::vector<uint8_t> &rawData) 
{
    if (!isRunning) return;
    m_SendQueue.Push(rawData);
    NetworkSleep(1);
}

std::optional<Packet> NetworkEngine::GetPacket()
{
    return m_RecvQueue.TryPop();
}

void NetworkEngine::FlushSendQueue()
{
    while (auto packetOpt = m_SendQueue.TryPop())
    {
        if(m_Transport)
            m_Transport->Send(packetOpt->data);
    }
}

void NetworkEngine::Start()
{
    isRunning = true;
    recvThread = std::thread(&NetworkEngine::RecvLoop, this);
    sendThread = std::thread(&NetworkEngine::SendLoop, this);
}

void NetworkEngine::Stop()
{
    isRunning = false;
    if (recvThread.joinable()) recvThread.join();
    if (sendThread.joinable()) sendThread.join();
}

void NetworkEngine::RecvLoop()
{
    // 현재는 Control Transport 수신만 담당한다고 가정
    // 필요시 Streaming 수신도 구현 가능하나, 영상 송신 위주이므로 생략
    while (isRunning)
    {
        // ... (기존 수신 로직 보완 필요 시 여기에 작성)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void NetworkEngine::SendLoop()
{
    while (isRunning)
    {
        FlushSendQueue();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}