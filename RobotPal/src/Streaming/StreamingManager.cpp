#include "RobotPal/Streaming/StreamingManager.h"
#include "RobotPal/Network/TransportFactory.h"
#include "stb_image_write.h"

#ifdef _WIN32
    #include <winsock2.h> // htonl용
#else
    #include <arpa/inet.h> // htonl용
#endif

void StreamingManager::write_func(void* ctx, void* data, int size) {
    auto* c = static_cast<WriteContext*>(ctx);
    const auto* byte_data = static_cast<const uint8_t*>(data);
    c->buffer.insert(c->buffer.end(), byte_data, byte_data + size);
}

// --- Emscripten static callbacks ---
#ifdef __EMSCRIPTEN__
static EM_BOOL on_open(int eventType, const EmscriptenWebSocketOpenEvent *e, void *userData) {
    if (userData) static_cast<StreamingManager*>(userData)->OnOpen();
    return EM_TRUE;
}

static EM_BOOL on_message(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData) {
    if (userData && !e->isText) {
        std::vector<uint8_t> data(e->data, e->data + e->numBytes);
        static_cast<StreamingManager*>(userData)->OnMessage(data);
    }
    return EM_TRUE;
}

static EM_BOOL on_error(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData) {
    if (userData) static_cast<StreamingManager*>(userData)->OnError();
    return EM_TRUE;
}

static EM_BOOL on_close(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData) {
    if (userData) static_cast<StreamingManager*>(userData)->OnClose();
    return EM_TRUE;
}
#endif
// ---

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

    // 2. 전송 (패킷 헤더 제거)
    // The Python server does not expect a length header, as WebSockets are already message-based.
    // We send the raw JPEG data directly.
    m_Transport->Send(ctx.buffer);
}

// --- Callback Implementations ---

void StreamingManager::OnOpen() {
    m_StatusMessage = "Connected!";
    m_IsConnected = true;
}

void StreamingManager::OnClose() {
    // m_ws = 0; // The socket is already deleted by the time this is called
    m_IsConnected = false;
    m_StatusMessage = "Connection Closed.";
}

void StreamingManager::OnError() {
    m_StatusMessage = "WebSocket Error.";
    m_IsConnected = false;
}

void StreamingManager::OnMessage(const std::vector<uint8_t>& data) {
    // We are a sender, but we can log received messages if needed
    // For now, do nothing.
}
