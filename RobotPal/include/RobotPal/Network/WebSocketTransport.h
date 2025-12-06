#pragma once
#include "RobotPal/Network/NetworkTransport.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

class WebSocketTransport : public NetworkTransport
{
public:
    WebSocketTransport();
    virtual ~WebSocketTransport();

    bool Connect(const std::string& url) override;
    void Disconnect() override;
    void Send(const std::vector<uint8_t>& data) override;
    bool IsConnected() const override { return m_IsConnected; }

private:
    static EM_BOOL OnOpen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData);
    static EM_BOOL OnClose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData);
    static EM_BOOL OnError(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData);
    static EM_BOOL OnMessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData);

    EMSCRIPTEN_WEBSOCKET_T m_Socket = 0;
    bool m_IsConnected = false;
};
#endif