#include "RobotPal/Network/TcpNetworkTransport.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cerrno>
#include "RobotPal/Util/Benchmark.h"
#ifndef __EMSCRIPTEN__
namespace {
uint32_t BenchmarkFrameId(const std::vector<uint8_t>& packet) {
    static const uint8_t magic[8] = {'R','P','B','E','N','C','H','1'};
    if (packet.size() < 24 || std::memcmp(packet.data() + 4, magic, 8) != 0) return 0;
    uint32_t id = 0;
    for (int i = 0; i < 4; ++i) id |= static_cast<uint32_t>(packet[12 + i]) << (i * 8);
    return id;
}
}
TcpNetworkTransport::TcpNetworkTransport()
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

TcpNetworkTransport::~TcpNetworkTransport()
{
    Disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool TcpNetworkTransport::Connect(const std::string& url)
{
    if (m_IsConnected) return true;

    Disconnect(); // Join workers from a previous remote close before reconnecting.

    // URL 파싱 (예: "127.0.0.1:9998")
    std::string ip = url;
    int port = 9998; // 기본 포트
    size_t colonPos = url.find(':');
    if (colonPos != std::string::npos) {
        ip = url.substr(0, colonPos);
        port = std::stoi(url.substr(colonPos + 1));
    }

    // 소켓 생성
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == INVALID_SOCKET) {
        std::cerr << "[TCP] Failed to create socket\n";
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    // 연결 시도 (Blocking)
    if (connect(m_Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[TCP] Connection Failed: " << url << "\n";
        CleanupSocket();
        return false;
    }

#ifdef SO_NOSIGPIPE
    int noSigpipe = 1;
    if (setsockopt(m_Socket, SOL_SOCKET, SO_NOSIGPIPE,
                   reinterpret_cast<const char*>(&noSigpipe), sizeof(noSigpipe)) == SOCKET_ERROR) {
        CleanupSocket();
        return false;
    }
#endif
    // Bound a stalled socket write. On timeout the whole connection is failed;
    // dropping a suffix would corrupt the receiver's length-prefixed stream.
#ifdef _WIN32
    DWORD timeout = 1000;
#else
    timeval timeout{1, 0};
#endif
    if (setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == SOCKET_ERROR) {
        CleanupSocket();
        return false;
    }

    std::cout << "[TCP] Connected to " << url << "\n";
    m_IsConnected = true;
    m_IsRunning = true;

    m_RecvThread = std::thread(&TcpNetworkTransport::RecvWorker, this);
    m_SendThread = std::thread(&TcpNetworkTransport::SendWorker, this);

    return true;
}

void TcpNetworkTransport::Disconnect()
{
    StopWorkers();
    if (m_RecvThread.joinable()) m_RecvThread.join();
    if (m_SendThread.joinable()) m_SendThread.join();
    // Keep the descriptor stable until both workers have stopped accessing it.
    CleanupSocket();

    // 큐 비우기
    m_SendQueue.Clear();
    std::cout << "[TCP] Disconnected\n";
}

void TcpNetworkTransport::Send(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_enqueueMutex);
    // [핵심] 큐에만 넣고 즉시 리턴 (Main Thread Blocking 방지)
    if (m_IsConnected) {
        const auto dropped = m_SendQueue.Dropped();
        const bool accepted = m_SendQueue.Push(data);
        if (!accepted || m_SendQueue.Dropped() != dropped)
            robotpal::benchmark::Event("transport_dropped", BenchmarkFrameId(data), 0,
                                      m_SendQueue.Dropped(), "bounded_drop_oldest_or_oversize");
        if (accepted) robotpal::benchmark::Event("transport_queued", BenchmarkFrameId(data), 0, m_SendQueue.Size());
    }
}

// --- Background Threads ---

void TcpNetworkTransport::RecvWorker()
{
    // 4KB 버퍼
    const int BUF_SIZE = 4096;
    std::vector<uint8_t> buffer(BUF_SIZE);

    while (m_IsRunning)
    {
        // recv는 데이터가 올 때까지 Blocking 됨 (CPU 소모 없음)
        // Disconnect()에서 소켓을 닫으면 SOCKET_ERROR 리턴하며 깨어남
        int bytesReceived = recv(m_Socket, (char*)buffer.data(), BUF_SIZE, 0);

        if (bytesReceived > 0)
        {
            // 실제 받은 크기만큼 잘라서 전달
            std::vector<uint8_t> packet(buffer.begin(), buffer.begin() + bytesReceived);
            
            // [부모 클래스 메서드 호출] 엔진으로 데이터 Push
            TriggerRecv(std::move(packet));
        }
        else if (bytesReceived == 0)
        {
            std::cout << "[TCP] Server closed connection.\n";
            m_IsConnected = false;
            break;
        }
        else
        {
            // 에러 혹은 소켓 닫힘
            if (m_IsRunning) {
                std::cerr << "[TCP] Recv Error\n";
            }
            break;
        }
    }
    StopWorkers();
}

void TcpNetworkTransport::SendWorker()
{
    while (m_IsRunning)
    {
        // 큐에서 패킷 꺼내기
        auto packetOpt = m_SendQueue.TryPop();

        if (packetOpt.has_value())
        {
            const auto& data = *packetOpt;
            const auto frameId = BenchmarkFrameId(data);
            const auto sendStart = robotpal::benchmark::SteadyNowNs();
            const bool completed = robotpal::SendComplete(data,
                [this](const uint8_t* bytes, size_t count) {
#ifdef _WIN32
                    const int sent = send(m_Socket, reinterpret_cast<const char*>(bytes), static_cast<int>(count), 0);
                    if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEINTR) return -2;
#else
                    const int sent = static_cast<int>(send(m_Socket, bytes, count,
#ifdef MSG_NOSIGNAL
                        MSG_NOSIGNAL
#else
                        0
#endif
                    ));
                    if (sent == SOCKET_ERROR && errno == EINTR) return -2;
#endif
                    return sent;
                }, [this] { return m_IsRunning.load(); });
            const auto sendNs = robotpal::benchmark::SteadyNowNs() - sendStart;
            if (!completed) {
                robotpal::benchmark::Event("send_failed", frameId, sendNs, 0, "closed_error_or_timeout");
                StopWorkers();
                break;
            }
            robotpal::benchmark::Event("send_completed", frameId, sendNs, data.size());
        }
        else
        {
            // 보낼 데이터가 없으면 살짝 잠들어서 CPU 점유율 낮춤
            // (NetworkQueue에 Condition Variable이 있다면 대기하는 게 더 좋음)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void TcpNetworkTransport::StopWorkers()
{
    std::lock_guard<std::mutex> lock(m_enqueueMutex);
    m_IsRunning = false;
    m_IsConnected = false;
    if (m_Socket != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(m_Socket, SD_BOTH);
#else
        shutdown(m_Socket, SHUT_RDWR);
#endif
    }
}

void TcpNetworkTransport::CleanupSocket()
{
    if (m_Socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_Socket);
#else
        close(m_Socket);
#endif
        m_Socket = INVALID_SOCKET;
    }
}
#endif
