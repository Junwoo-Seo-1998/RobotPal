#include "RobotPal/Network/WebSocketTransport.h"


#ifdef __EMSCRIPTEN__
#include <iostream>
WebSocketTransport::WebSocketTransport() = default;
WebSocketTransport::~WebSocketTransport()
{
    Disconnect();
}

bool WebSocketTransport::Connect(const std::string& url)
{
    if (m_Socket > 0)
    {
        return true; // Already connected or connecting
    }

    EmscriptenWebSocketCreateAttributes ws_attrs =
    {
        url.c_str(),
        nullptr,
        EM_TRUE
    };

    m_Socket = emscripten_websocket_new(&ws_attrs);

    if (m_Socket <= 0)
    {
        std::cerr << "WebSocket creation failed." << std::endl;
        return false;
    }

    emscripten_websocket_set_onopen_callback(m_Socket, this, OnOpen);
    emscripten_websocket_set_onclose_callback(m_Socket, this, OnClose);
    emscripten_websocket_set_onerror_callback(m_Socket, this, OnError);
    emscripten_websocket_set_onmessage_callback(m_Socket, this, OnMessage);

    return true;
}

void WebSocketTransport::Disconnect()
{
    if (m_Socket > 0)
    {
        emscripten_websocket_close(m_Socket, 1000, "Closing connection");
        emscripten_websocket_delete(m_Socket);
        m_Socket = 0;
    }
    m_IsConnected = false;
}

void WebSocketTransport::Send(const std::vector<uint8_t>& data)
{
    if (IsConnected())
    {
        EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(m_Socket, (void*)data.data(), data.size());
        if (result != EMSCRIPTEN_RESULT_SUCCESS) {
            std::cerr << "Failed to send WebSocket binary message. Error code: " << result << std::endl;
        }
    }
}

EM_BOOL WebSocketTransport::OnOpen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData)
{
    auto* transport = static_cast<WebSocketTransport*>(userData);
    if (transport)
    {
        std::cout << "WebSocket connection opened." << std::endl;
        transport->m_IsConnected = true;
    }
    return EM_TRUE;
}

EM_BOOL WebSocketTransport::OnClose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData)
{
    auto* transport = static_cast<WebSocketTransport*>(userData);
    if (transport)
    {
        std::cout << "WebSocket connection closed." << std::endl;
        transport->m_IsConnected = false;
        transport->m_Socket = 0;
    }
    return EM_TRUE;
}

EM_BOOL WebSocketTransport::OnError(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData)
{
    auto* transport = static_cast<WebSocketTransport*>(userData);
    if (transport)
    {
        std::cerr << "WebSocket error." << std::endl;
        transport->m_IsConnected = false;
        transport->m_Socket = 0;
    }
    return EM_TRUE;
}

EM_BOOL WebSocketTransport::OnMessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData)
{
    // This transport is mainly for sending, but we can handle incoming if needed.
    // Acknowledgment frames, etc.
    if (websocketEvent->isText) {
        printf("onmessage: %s\n", websocketEvent->data);
    } else {
        printf("onmessage: data len: %d\n", websocketEvent->numBytes);

    }
    return EM_TRUE;
}

#endif // __EMSCRIPTEN__