#pragma once
#include "RobotPal/Streaming/IStreamingManager.h"
#include "RobotPal/Network/INetworkTransport.h"
#include <flecs.h>
#include <memory>
#include <vector>
#include <functional>

class StreamingManager : public IStreamingManager
{
public:
    StreamingManager(flecs::world& world);
    virtual ~StreamingManager();

    void Init() override;
    void Shutdown() override;

    // [삭제] Connect/Disconnect 오버라이드 제거

    void SendFrame(const FrameData& frame) override;
private:
    flecs::world& m_World;
    struct WriteContext {
        std::vector<uint8_t> buffer;
    };
    static void write_func(void* ctx, void* data, int size);
};