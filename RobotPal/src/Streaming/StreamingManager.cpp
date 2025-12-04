#include "RobotPal/Streaming/StreamingManager.h"
#include "stb_image_write.h"
#include "Robotpal/Components/Components.h"
#include "RobotPal/Network/NetworkEngine.h"

#ifdef _WIN32
    #include <winsock2.h> // htonl용
#else
    #include <arpa/inet.h> // htonl용
#endif

StreamingManager::StreamingManager(flecs::world& world) 
    : m_World(world) 
{
}

StreamingManager::~StreamingManager() {
    Shutdown();
}

void StreamingManager::Init() {
}

void StreamingManager::Shutdown() {
}

// [삭제] Connect/Disconnect 구현 제거

void StreamingManager::SendFrame(const FrameData& frame) {
    auto& handle = m_World.get_mut<NetworkEngineHandle>();

    WriteContext ctx;
    ctx.buffer.reserve(frame.width * frame.height);

    int ok = stbi_write_jpg_to_func(
        write_func, &ctx,
        frame.width, frame.height, frame.channels,
        frame.pixel_data.data(), 85
    );

    if (!ok || ctx.buffer.empty()) return;

    // 길이 헤더(4바이트) + JPEG 데이터
    std::vector<uint8_t> packet;
    uint32_t size_be = htonl(static_cast<uint32_t>(ctx.buffer.size()));
    const uint8_t* size_ptr = reinterpret_cast<const uint8_t*>(&size_be);

    packet.insert(packet.end(), size_ptr, size_ptr + 4);
    packet.insert(packet.end(), ctx.buffer.begin(), ctx.buffer.end());

    handle.instance->SendPacket(packet);
}

void StreamingManager::write_func(void* ctx, void* data, int size) {
    auto* c = static_cast<WriteContext*>(ctx);
    c->buffer.insert(c->buffer.end(), (uint8_t*)data, (uint8_t*)data + size);
}