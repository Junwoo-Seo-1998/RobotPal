#ifndef __STREAMINGMANAGER_H__
#define __STREAMINGMANAGER_H__

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#ifdef __EMSCRIPTEN__
#include <emscripten/websocket.h>
#else
// Define placeholder for non-emscripten builds if needed
using EMSCRIPTEN_WEBSOCKET_T = int;
#endif

class StreamingManager
{
public:
    StreamingManager();
    ~StreamingManager();

    void Init();
    void Shutdown();

    void ConnectToServer(const std::string& url);
    void Disconnect();

    // Sends a frame by compressing it to JPEG in memory first
    void SendFrame(const std::vector<uint8_t>& pixel_data, int width, int height, int channels);

    bool IsConnected() const { return m_IsConnected; }
    const std::string& GetStatusMessage() const { return m_StatusMessage; }

    // Public callbacks for Emscripten
    void OnOpen();
    void OnClose();
    void OnError();
    void OnMessage(const std::vector<uint8_t>& data);


private:
    std::string m_StatusMessage = "Ready";
    bool m_IsConnected = false;

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBSOCKET_T m_ws = 0;
#else
    int m_sock = -1; // Placeholder for native socket
#endif
};

#endif