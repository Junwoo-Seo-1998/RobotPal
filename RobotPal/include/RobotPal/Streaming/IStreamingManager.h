#ifndef __ISTREAMINGMANAGER_H__
#define __ISTREAMINGMANAGER_H__

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
struct FrameData {
    std::vector<uint8_t> pixel_data;
    int width = 0;
    int height = 0;
    int channels = 0;
};

class IStreamingManager
{
public:
    virtual ~IStreamingManager() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void ConnectToServer(const std::string& url) = 0;
    virtual void Disconnect() = 0;
    virtual void SendFrame(const FrameData& frame) = 0;

    bool IsConnected() const { return m_IsConnected; }
    const std::string& GetStatusMessage() const { return m_StatusMessage; }

    // -----------------------------
    // Static Factory
    // -----------------------------
    enum class StreamingType {
        WebSocket,
        TCP
    };

    static std::shared_ptr<IStreamingManager> Create(StreamingType type);
protected:
    std::string m_StatusMessage = "Ready";
    bool m_IsConnected = false;
};

#endif
