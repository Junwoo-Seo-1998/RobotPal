#include "RobotPal/Util/StreamingPipeline.h"

#include "RobotPal/Util/JpegEncoder.h"
#include "RobotPal/Util/Benchmark.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

// ---------------------------------------------
// Thread config (Native / Emscripten)
// ---------------------------------------------
struct StreamingThreadConfig {
    unsigned encodeWorkers;
};

static StreamingThreadConfig GetStreamingThreadConfig() {
    const char* configured = std::getenv("ROBOTPAL_ENCODE_WORKERS");
    if (configured) {
        const int workers = std::atoi(configured);
        if (workers == 1 || workers == 4) return {static_cast<unsigned>(workers)};
    }
#ifdef __EMSCRIPTEN__
    #ifdef __EMSCRIPTEN_PTHREADS__
        unsigned max_web_workers = 8; 
        return {std::max(1u, max_web_workers - 2)};
    #else
        return {1};
    #endif
#else
    // Native TCP
    return {4};
#endif
}


StreamingPipeline::StreamingPipeline(flecs::world& world) {
    auto handle = world.get_mut<const NetworkEngineHandle>();
    m_networkEngine = handle.instance;

    auto cfg = GetStreamingThreadConfig();
    std::cout << cfg.encodeWorkers << " encode workers " << std::endl;
    m_isRunning.store(true);

    // Encode workers
    for (unsigned i = 0; i < cfg.encodeWorkers; ++i) {
        m_encodeThreads.emplace_back(
            &StreamingPipeline::EncodeWorkerLoop,
            this
        );
    }
}

StreamingPipeline::~StreamingPipeline() {
    m_isRunning.store(false);

    m_encodeQueueCv.notify_all(); // Thread 루프 탈출

    for (auto& t : m_encodeThreads) {
        if (t.joinable())
            t.join();
    }
}

// ---------------------------------------------
// Public API
// ---------------------------------------------
void StreamingPipeline::TryConnect(const std::string& url) {
    if (m_networkEngine)
        m_networkEngine->TryConnect(url);
}

void StreamingPipeline::PushFrame(
    std::vector<uint8_t>&& pixels,
    int width,
    int height,
    int components,
    uint32_t frameId,
    uint64_t generatedUnixNs,
    uint64_t queueStartNs
) {
    FrameEncodeJob job;
    job.pixels = std::move(pixels);
    job.width = width;
    job.height = height;
    job.components = components;
    job.frameId = frameId;
    job.generatedUnixNs = generatedUnixNs;
    job.queueStartNs = queueStartNs;

    {
        std::lock_guard<std::mutex> lk(m_encodeQueueMutex);
        if (m_encodeQueue.size() >= EncodeQueueCapacity) {
            const auto droppedFrameId = m_encodeQueue.front().frameId;
            m_encodeQueue.pop();
            robotpal::benchmark::Event(
                "encode_queue_dropped",
                droppedFrameId,
                0,
                EncodeQueueCapacity,
                "drop_oldest_capacity_6"
            );
        }
        m_encodeQueue.push(std::move(job));
        robotpal::benchmark::Event("encode_queued", frameId, 0, m_encodeQueue.size());
    }
    m_encodeQueueCv.notify_one();
}

// ---------------------------------------------
// Encode worker (Optimized)
// ---------------------------------------------
void StreamingPipeline::EncodeWorkerLoop() {
    // JpegEncoder 생성
    JpegEncoder* jpeg = CreateJpegEncoder();

    // [Optimized] 루프 외부에서 버퍼 할당 (메모리 재사용)
    std::vector<uint8_t> rgbBuffer;
    std::vector<uint8_t> jpegOut;
    std::vector<uint8_t> packet;
    
    // 예상되는 최대 버퍼 크기 예약 (예: 1MB)
    // 해상도에 따라 다를 수 있으나 빈번한 재할당 방지용
    rgbBuffer.reserve(2000 * 2000 * 4);
    jpegOut.reserve(1024 * 1024);
    packet.reserve(1024 * 1024 + 128);

    while (m_isRunning.load()) {
        FrameEncodeJob job;
        {
            std::unique_lock<std::mutex> lk(m_encodeQueueMutex);
            m_encodeQueueCv.wait(lk, [&] {
                return !m_encodeQueue.empty() || !m_isRunning.load();
            });

            if (!m_isRunning.load() && m_encodeQueue.empty())
                break; // return 대신 break로 리소스 정리 유도

            job = std::move(m_encodeQueue.front());
            m_encodeQueue.pop();
            robotpal::benchmark::Event("encode_dequeued", job.frameId, robotpal::benchmark::SteadyNowNs() - job.queueStartNs, m_encodeQueue.size());
        }

        const uint8_t* rgbPtr = nullptr;

        // RGB 변환 또는 포인터 설정
        if (job.components == 3) {
            rgbPtr = job.pixels.data();
        } else if (job.components == 4) {
            size_t pixelCount = (size_t)job.width * (size_t)job.height;
            size_t requiredSize = pixelCount * 3;

            // 필요시 버퍼 크기 증가 (capacity 내라면 재할당 없음)
            if(rgbBuffer.size() < requiredSize) 
                rgbBuffer.resize(requiredSize);

            const uint8_t* s = job.pixels.data();
            uint8_t* d = rgbBuffer.data();

            // RGBA -> RGB 변환
            for (size_t i = 0; i < pixelCount; ++i) {
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                d += 3;
                s += 4;
            }
            rgbPtr = rgbBuffer.data();
        } else {
            robotpal::benchmark::Event("encode_failed", job.frameId, 0, 0, "unsupported_components");
            continue;
        }

        // JPEG 인코딩
        // [Optimized] 기존 벡터 재사용 (clear만 호출)
        jpegOut.clear();
        
        const auto t0 = robotpal::benchmark::SteadyNowNs();
        bool ok = jpeg->EncodeRGB(
            rgbPtr,
            job.width,
            job.height,
            70,
            jpegOut
        );
        const auto encodeNs = robotpal::benchmark::SteadyNowNs() - t0;

        if (!ok) {
            robotpal::benchmark::Event("encode_failed", job.frameId, encodeNs, 0, "jpeg");
            continue;
        }
        robotpal::benchmark::Event("encode_completed", job.frameId, encodeNs, jpegOut.size());

        // Packet 생성: [Header 4bytes] + [JPEG Body]
        uint32_t len = static_cast<uint32_t>(jpegOut.size());
        
        // [Optimized] 패킷 버퍼 재사용
        packet.clear();
        
        // 헤더 공간 + 데이터 공간 확보
        // reserve는 현재 capacity보다 클 때만 재할당하므로 안전
        constexpr uint32_t metadataSize = 20;
        const bool benchmarkEnabled = robotpal::benchmark::EventLog::Instance().Enabled();
        const uint32_t bodyLen = len + (benchmarkEnabled ? metadataSize : 0);
        if (packet.capacity() < 4 + bodyLen) {
            packet.reserve(4 + bodyLen);
        }

        // 헤더
        packet.push_back(bodyLen & 0xFF);
        packet.push_back((bodyLen >> 8) & 0xFF);
        packet.push_back((bodyLen >> 16) & 0xFF);
        packet.push_back((bodyLen >> 24) & 0xFF);
        if (benchmarkEnabled) {
            const uint8_t magic[8] = {'R','P','B','E','N','C','H','1'};
            packet.insert(packet.end(), magic, magic + 8);
            for (int shift = 0; shift < 4; ++shift) packet.push_back(static_cast<uint8_t>((job.frameId >> (shift * 8)) & 0xff));
            for (int shift = 0; shift < 8; ++shift) packet.push_back(static_cast<uint8_t>((job.generatedUnixNs >> (shift * 8)) & 0xff));
        }
        
        // 데이터 (insert 최적화 활용)
        packet.insert(packet.end(), jpegOut.begin(), jpegOut.end());

        if(m_networkEngine) {
            // NetworkEngine::SendPacket은 const std::vector&를 받으므로 
            // 복사가 발생하지만, 여기서 packet 인스턴스 자체는 유지되어 
            // 다음 루프에서 재사용됨.
            m_networkEngine->SendPacket(packet);
            robotpal::benchmark::Event("send_queued", job.frameId, 0, packet.size());
        }
    }
}
