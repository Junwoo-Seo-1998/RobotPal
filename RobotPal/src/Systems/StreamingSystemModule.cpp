#include "RobotPal/Systems/StreamingSystemModule.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Network/NetworkEngine.h"
#include "stb_image_write.h"
#include <iostream>
StreamingSystemModule::StreamingSystemModule(flecs::world& world)
{
    bool networkEngineFound=false;
    auto handle = world.get_mut<const NetworkEngineHandle>();

    netEngine=handle.instance;

    if(!networkEngineFound)
    {
        std::cout<<"check module init order - network engine was not started\n";
    }
    
    RegisterObserver(world);    
    RegisterSystem(world);
}

// struct config{
//     #ifdef __EMSCRIPTEN__
//     int port;


void StreamingSystemModule::RegisterObserver(flecs::world& world)
{
    world.observer<const VideoSender>()
    .event(flecs::OnSet)
    .each([&](flecs::entity e, const VideoSender &videoCmp)
    {
        netEngine->TryConnect(videoCmp.url);
    });
}

struct WriteContext 
{
    std::vector<uint8_t> buffer;
};

static void write_func(void* ctx, void* data, int size) {
    auto* c = static_cast<WriteContext*>(ctx);
    c->buffer.insert(c->buffer.end(), (uint8_t*)data, (uint8_t*)data + size);
}

void StreamingSystemModule::RegisterSystem(flecs::world& world)
{
    world.system<const Camera, const RenderTarget, const VideoSender>()
    .kind(flecs::OnStore)
    .each([&](flecs::entity entity, const Camera &/*cam*/, const RenderTarget &renderTarget, const VideoSender& videoCmp)
    {
        auto data=renderTarget.fbo->GetColorAttachment()->GetAsyncData();
        if (!data.empty())
        {
            auto width = renderTarget.fbo->GetWidth();
            auto height = renderTarget.fbo->GetHeight();
            // The texture format is RGBA, so 3 channels.
            //m_StreamingManager->SendFrame({data, width, height, 3});
            {
                WriteContext ctx;
                ctx.buffer.reserve(width * height);

                int ok = stbi_write_jpg_to_func(
                    write_func, &ctx,
                    width, height, 3, //웹소켓이랑 형식 맞추세요
                    data.data(), 85
                    
                );

                if (!ok || ctx.buffer.empty()) return;

                // 길이 헤더(4바이트) + JPEG 데이터
                std::vector<uint8_t> packet;

//                OPENCV RGB BGR 맞아 그거떄문임// 이거 그럼 그냥 서버에서 해도됨? ㅇㅋ?
                // uint32_t size_be = htonl(static_cast<uint32_t>(ctx.buffer.size()));
                // const uint8_t* size_ptr = reinterpret_cast<const uint8_t*>(&size_be);

                // packet.insert(packet.end(), size_ptr, size_ptr + 4);
                // packet.insert(packet.end(), ctx.buffer.begin(), ctx.buffer.end());
                //netEngine->SendPacket();
            }
        }
    });
}