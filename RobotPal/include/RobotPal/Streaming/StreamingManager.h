#pragma once
#include "RobotPal/Streaming/IStreamingManager.h"
#include "RobotPal/Network/INetworkTransport.h"
#include <memory>
#include <vector>

class StreamingManager : public IStreamingManager
{
public:
    StreamingManager();
    virtual ~StreamingManager();

    void Init() override;
    void Shutdown() override;

    void ConnectToServer(const std::string& url) override;
    void Disconnect() override;
    void SendFrame(const FrameData& frame) override;

    bool IsConnected() const { return m_IsConnected; }
    const std::string& GetStatusMessage() const { return m_StatusMessage; }

    // Public callbacks for Emscripten
    void OnOpen();
    void OnClose();
    void OnError();
    void OnMessage(const std::vector<uint8_t>& data);

private:
    std::unique_ptr<INetworkTransport> m_Transport;

    // JPEG 압축용 컨텍스트
    struct WriteContext {
        std::vector<uint8_t> buffer;
    };
    static void write_func(void* ctx, void* data, int size);
};