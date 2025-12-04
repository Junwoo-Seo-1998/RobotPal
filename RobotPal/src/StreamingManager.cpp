#include "RobotPal/StreamingManager.h"
#include <iostream>
#include <vector>

// STB Image Write for compressing frames to JPEG
#include "stb_image_write.h"

// Non-Emscripten builds might need socket headers, but we'll focus on Emscripten
#ifndef __EMSCRIPTEN__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Helper struct for stb_image_write context
struct WriteContext {
    std::vector<uint8_t> buffer;
};

// Callback for stbi_write_jpg_to_func
static void write_func(void *context, void *data, int size) {
    auto* write_context = static_cast<WriteContext*>(context);
    const auto* bytes = static_cast<const uint8_t*>(data);
    write_context->buffer.insert(write_context->buffer.end(), bytes, bytes + size);
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

StreamingManager::StreamingManager() = default;
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
#ifdef __EMSCRIPTEN__
    if (m_ws > 0) return;

    EmscriptenWebSocketCreateAttributes ws_attrs = {url.c_str(), nullptr, EM_TRUE};
    m_ws = emscripten_websocket_new(&ws_attrs);
    if (m_ws <= 0) {
        m_StatusMessage = "Failed to create WebSocket.";
        return;
    }

    emscripten_websocket_set_onopen_callback(m_ws, this, on_open);
    emscripten_websocket_set_onmessage_callback(m_ws, this, on_message);
    emscripten_websocket_set_onclose_callback(m_ws, this, on_close);
    emscripten_websocket_set_onerror_callback(m_ws, this, on_error);

    m_StatusMessage = "Connecting...";
#else
    m_StatusMessage = "Native sockets not implemented in this version.";
#endif
}

void StreamingManager::Disconnect() {
#ifdef __EMSCRIPTEN__
    if (m_ws > 0) {
        emscripten_websocket_close(m_ws, 1000, "Shutting down");
        emscripten_websocket_delete(m_ws);
    }
#endif
    m_ws = 0;
    m_IsConnected = false;
    m_StatusMessage = "Disconnected";
}

void StreamingManager::SendFrame(const std::vector<uint8_t>& pixel_data, int width, int height, int channels) {
#ifdef __EMSCRIPTEN__
    if (!m_IsConnected || m_ws <= 0 || pixel_data.empty()) {
        return;
    }

    WriteContext context;
    // Pre-allocate some memory to avoid frequent re-allocations
    context.buffer.reserve(width * height); 

    const int quality = 85; // JPEG quality (0-100)
    
    stbi_flip_vertically_on_write(1);
    // stbi_write_jpg_to_func writes the JPEG into our vector via the callback
    int success = stbi_write_jpg_to_func(
        write_func,
        &context,
        width,
        height,
        channels,
        pixel_data.data(),
        quality
    );

    if (success && !context.buffer.empty()) {
        emscripten_websocket_send_binary(m_ws, context.buffer.data(), context.buffer.size());
    } else {
        std::cerr << "Failed to compress frame to JPEG." << std::endl;
    }
#endif
}

// --- Callback Implementations ---

void StreamingManager::OnOpen() {
    m_StatusMessage = "Connected!";
    m_IsConnected = true;
}

void StreamingManager::OnClose() {
    m_ws = 0; // The socket is already deleted by the time this is called
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