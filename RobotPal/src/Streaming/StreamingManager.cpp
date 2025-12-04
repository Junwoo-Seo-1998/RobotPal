#include "RobotPal/Streaming/StreamingManager.h"
#include "RobotPal/Network/TransportFactory.h"
#include "stb_image_write.h"

#ifdef _WIN32
    #include <winsock2.h> // htonl용
#else
    #include <arpa/inet.h> // htonl용
#endif

StreamingManager::StreamingManager() {
    // [핵심] 팩토리를 통해 생성 (Web/Native 자동 분기)
    m_Transport = TransportFactory::Create();
}

StreamingManager::~StreamingManager() {
    Shutdown();
}

void StreamingManager::Init() {
    m_StatusMessage = "Ready";
    m_IsConnected = false;
}

void StreamingManager::Shutdown() {
    Disconnect();
}

void StreamingManager::ConnectToServer(const std::string& url) {
    if (!m_Transport) return;

    if (m_Transport->Connect(url)) {
        m_IsConnected = true;
        m_StatusMessage = "Connected";
    } else {
        m_IsConnected = false;
        m_StatusMessage = "Connection Failed";
    }
}

void StreamingManager::Disconnect() {
    if (m_Transport) {
        m_Transport->Disconnect();
    }
    m_IsConnected = false;
    m_StatusMessage = "Disconnected";
}

void StreamingManager::SendFrame(const FrameData& frame) {
    if (!m_Transport || !m_Transport->IsConnected()) {
        // Transport 상태와 동기화
        if (m_IsConnected) { 
            m_IsConnected = false; 
            m_StatusMessage = "Disconnected (Transport lost)";
        }
        return;
    }

    if (frame.pixel_data.empty()) return;

    // 1. JPEG 압축
    WriteContext ctx;
    // 대략적인 크기 예약
    ctx.buffer.reserve(frame.width * frame.height);

    int ok = stbi_write_jpg_to_func(
        write_func, &ctx,
        frame.width, frame.height, frame.channels,
        frame.pixel_data.data(), 85
    );

    if (!ok || ctx.buffer.empty()) return;

    // 2. 패킷 헤더 (길이) + 본문 합치기
    std::vector<uint8_t> packet;
    uint32_t size_be = htonl(static_cast<uint32_t>(ctx.buffer.size()));
    const uint8_t* size_ptr = reinterpret_cast<const uint8_t*>(&size_be);

    packet.insert(packet.end(), size_ptr, size_ptr + 4);
    packet.insert(packet.end(), ctx.buffer.begin(), ctx.buffer.end());

    // 3. 전송
    m_Transport->Send(packet);
}

void StreamingManager::write_func(void* ctx, void* data, int size) {
    auto* c = static_cast<WriteContext*>(ctx);
    c->buffer.insert(c->buffer.end(), (uint8_t*)data, (uint8_t*)data + size);
}