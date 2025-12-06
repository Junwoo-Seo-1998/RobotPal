#include "RobotPal/Systems/StreamingSystemModule.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Network/NetworkEngine.h"
#include "stb_image_write.h"
#include <iostream>
#include "imgui.h"
#include <glad/gles2.h>
StreamingSystemModule::StreamingSystemModule(flecs::world& world)
{
    bool networkEngineFound=false;
    auto networkEngine = world.get<const NetworkEngineHandle>();

    netEngine=networkEngine.instance;

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
    .kind(flecs::PostFrame) // [핵심] 렌더링(OnStore)이 끝난 직후 실행
    .multi_threaded(false)
    .each([&](flecs::entity entity, const Camera &cam, const RenderTarget &renderTarget, const VideoSender& videoCmp)
    {
        auto fbo=renderTarget.fbo;
        auto tex=fbo->GetColorAttachment();

        uint64_t currentFrame = world.get_info()->frame_count_total;
        auto data=tex->GetAsyncData(currentFrame);
        if (!data.empty())
        {
            auto colorTex = renderTarget.fbo->GetColorAttachment();
            auto width = colorTex->GetWidth();
            auto height = colorTex->GetHeight();
            auto format = colorTex->GetFormat();

            int components = 0;
            if (format == TextureFormat::RGB8)
                components = 3;
            else if (format == TextureFormat::RGBA8)
                components = 4;
            else
                return; // 포맷 지원 안함
            
            WriteContext ctx;
            ctx.buffer.reserve(width * height*3);

            int ok = stbi_write_jpg_to_func(
                write_func, &ctx,
                width, height, components,
                data.data(), 85
                
            );
            if (!ok || ctx.buffer.empty()) return;
            netEngine->SendPacket(ctx.buffer);
            
        }
    });
}