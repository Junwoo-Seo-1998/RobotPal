#include "RobotPal/Systems/StreamingSystemModule.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Util/StreamingPipeline.h"
#include "RobotPal/Util/Benchmark.h"
#include <cstdlib>
#include <string>

StreamingSystemModule::StreamingSystemModule(flecs::world& world)
    : m_world(world)
{
    m_worker = std::make_unique<StreamingPipeline>(world);

    RegisterObserver(world);
    RegisterSystem(world);
}

StreamingSystemModule::~StreamingSystemModule() = default;

void StreamingSystemModule::RegisterObserver(flecs::world& world)
{
    world.observer<const VideoSender>()
        .event(flecs::OnSet)
        .each([this](flecs::entity, const VideoSender& sender) {
            if (m_worker && sender.url.size()) {
                m_worker->TryConnect(sender.url);
            }
        });
}

void StreamingSystemModule::RegisterSystem(flecs::world& world)
{
    world.system<const Camera, const RenderTarget, const VideoSender>()
        .kind(flecs::PostFrame)
        .rate(1)
        .each([this](flecs::entity, const Camera&, const RenderTarget& rt, const VideoSender&) {
            if(!m_world.get_mut<const NetworkEngineHandle>().instance->IsConnected()) return;
            const auto frameId = static_cast<uint32_t>(m_world.get_info()->frame_count_total);
            const auto generatedUnixNs = robotpal::benchmark::UnixNowNs();
            robotpal::benchmark::Event("frame_generated", frameId);
            auto tex = rt.fbo->GetColorAttachment();
            const char* configuredMode = std::getenv("ROBOTPAL_READBACK_MODE");
            const bool synchronous = configuredMode && std::string(configuredMode) == "sync";
            const auto readbackStart = robotpal::benchmark::SteadyNowNs();
            auto raw = synchronous
                ? tex->GetSyncData(m_world.get_info()->frame_count_total)
                : tex->GetAsyncData(m_world.get_info()->frame_count_total);
            const auto readbackNs = robotpal::benchmark::SteadyNowNs() - readbackStart;
            if (raw.empty()) { robotpal::benchmark::Event("readback_failed", frameId, readbackNs, 0, "empty"); return; }
            uint32_t sourceFrameId = frameId;
            uint64_t sourceGeneratedUnixNs = generatedUnixNs;
            if (!synchronous) {
                if (!m_pboPrimed) {
                    m_pboPrimed = true;
                    m_pendingPboFrameId = frameId;
                    m_pendingPboGeneratedUnixNs = generatedUnixNs;
                    robotpal::benchmark::Event("readback_pending", frameId, readbackNs, 0, "pbo_warmup");
                    return;
                }
                sourceFrameId = m_pendingPboFrameId;
                sourceGeneratedUnixNs = m_pendingPboGeneratedUnixNs;
                m_pendingPboFrameId = frameId;
                m_pendingPboGeneratedUnixNs = generatedUnixNs;
            }
            robotpal::benchmark::Event("readback_completed", sourceFrameId, readbackNs, raw.size());

            int comps =
                tex->GetFormat() == TextureFormat::RGB8 ? 3 :
                tex->GetFormat() == TextureFormat::RGBA8 ? 4 : 0;
            if (!comps) return;

            m_worker->PushFrame(
                std::move(raw),
                tex->GetWidth(),
                tex->GetHeight(),
                comps,
                sourceFrameId,
                sourceGeneratedUnixNs,
                robotpal::benchmark::SteadyNowNs()
            );
        });
}
