#include "RobotPal/Network/NetworkEngine.h"



NetworkEngine::NetworkEngine(flecs::world &world)
    : m_World(world), m_SendQueue(), m_RecvQueue(), isRunning(false), recvThread(), sendThread()
{
    m_World.set<NetworkEngineHandle>({this});
    m_World.set<NetworkConnectionState>({ConnectionStatus::Disconnected, "", 0.0f});
}

NetworkEngine::~NetworkEngine()
{
    Disconnect();
}

void NetworkEngine::Connect(const std::string &url)
{
    if(isRunning) return;

    std::string ip = url; // 나중에 URL 파싱 필요

    Start();
}

void NetworkEngine::Disconnect()
{
    Stop();
}

void NetworkEngine::SendPacket(const std::vector<uint8_t> &rawData)
{
    if (!isRunning)
        return;
    std::vector<uint8_t> packet = rawData;
    // 아직은 패킷 포장이없는데 나중에 추가해야할수도 있음

    m_SendQueue.Push(packet);
}

std::optional<Packet> NetworkEngine::GetPacket()
{
    return m_RecvQueue.TryPop();
}

void NetworkEngine::FlushSendQueue()
{
    while (auto packetOpt = m_SendQueue.TryPop())
    {
        // 실제 소켓 전송은 여기서 몰아서 처리
        // Emscripten 웹소켓이나 TCP 소켓의 send 호출
    }
}

void NetworkEngine::Start()
{
    isRunning = true;

    // 1. 수신 스레드 시작
    recvThread = std::thread(&NetworkEngine::RecvLoop, this);

    // 2. 송신 스레드 시작
    sendThread = std::thread(&NetworkEngine::SendLoop, this);
}

void NetworkEngine::Stop()
{
    isRunning = false;
    if (recvThread.joinable())
        recvThread.join();
    if (sendThread.joinable())
        sendThread.join();
}

void NetworkEngine::RecvLoop()
{
    while (isRunning)
    {
        // int len = recv(socket, buffer, ...);

        // if (len > 0)
        // {
        //    // m_RecvQueue.Push();
        // }
    }
}

void NetworkEngine::SendLoop()
{
    while (isRunning)
    {
        FlushSendQueue();
    }
}