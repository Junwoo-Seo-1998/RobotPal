#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "RobotPal/Network/NetworkEngine.h"

class StreamingPipeline {
public:
    static constexpr size_t EncodeQueueCapacity = 6;

    StreamingPipeline(flecs::world& world);
    ~StreamingPipeline();

    void PushFrame(
        std::vector<uint8_t>&& pixels,
        int width,
        int height,
        int components,
        uint32_t frameId,
        uint64_t generatedUnixNs,
        uint64_t queueStartNs
    );

    void TryConnect(const std::string& url);

private:
    struct FrameEncodeJob {
        std::vector<uint8_t> pixels;
        int width;
        int height;
        int components;
        uint32_t frameId;
        uint64_t generatedUnixNs;
        uint64_t queueStartNs;
    };

    void EncodeWorkerLoop();

private:
    NetworkEngine* m_networkEngine = nullptr;
    std::atomic<bool> m_isRunning{false};

    std::queue<FrameEncodeJob> m_encodeQueue;
    std::mutex m_encodeQueueMutex;
    std::condition_variable m_encodeQueueCv;

    std::vector<std::thread> m_encodeThreads;
};
