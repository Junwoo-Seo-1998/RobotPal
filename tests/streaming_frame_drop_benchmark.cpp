#include "RobotPal/Util/JpegEncoder.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>

struct Job {
    std::vector<uint8_t> rgba;
};

struct Result {
    int workerCount = 0;
    int produced = 0;
    int dropped = 0;
    int processed = 0;
    double elapsedSec = 0.0;
    double inputFps = 0.0;
    double outputFps = 0.0;
    double dropRate = 0.0;
};

Result RunScenario(int workerCount, int width, int height, int quality, int inputFps, int durationSec, size_t maxQueueSize) {
    JpegEncoder* encoder = CreateJpegEncoder();

    std::deque<Job> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> producerDone{false};

    std::atomic<int> produced{0};
    std::atomic<int> dropped{0};
    std::atomic<int> processed{0};

    const auto start = std::chrono::steady_clock::now();
    const auto endTime = start + std::chrono::seconds(durationSec);
    const auto frameInterval = std::chrono::microseconds(1'000'000 / std::max(1, inputFps));

    std::vector<uint8_t> baseRgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    for (size_t i = 0; i < baseRgba.size(); ++i) {
        baseRgba[i] = static_cast<uint8_t>((i * 131u) & 0xFFu);
    }

    std::thread producer([&]() {
        auto nextTick = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() < endTime) {
            Job job;
            job.rgba = baseRgba;

            {
                std::lock_guard<std::mutex> lk(mtx);
                if (queue.size() >= maxQueueSize) {
                    queue.pop_front();
                    dropped.fetch_add(1, std::memory_order_relaxed);
                }
                queue.push_back(std::move(job));
                produced.fetch_add(1, std::memory_order_relaxed);
            }
            cv.notify_one();

            nextTick += frameInterval;
            std::this_thread::sleep_until(nextTick);
        }

        producerDone.store(true, std::memory_order_release);
        cv.notify_all();
    });

    std::vector<std::thread> workers;
    workers.reserve(static_cast<size_t>(workerCount));

    for (int w = 0; w < workerCount; ++w) {
        workers.emplace_back([&]() {
            std::vector<uint8_t> rgb;
            std::vector<uint8_t> jpeg;
            rgb.reserve(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
            jpeg.reserve(1024 * 1024);

            while (true) {
                Job job;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cv.wait(lk, [&]() { return !queue.empty() || producerDone.load(std::memory_order_acquire); });
                    if (queue.empty()) {
                        if (producerDone.load(std::memory_order_acquire)) {
                            break;
                        }
                        continue;
                    }
                    job = std::move(queue.front());
                    queue.pop_front();
                }

                const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
                rgb.resize(pixelCount * 3);
                const uint8_t* s = job.rgba.data();
                uint8_t* d = rgb.data();
                for (size_t i = 0; i < pixelCount; ++i) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d += 3;
                    s += 4;
                }

                jpeg.clear();
                const bool ok = encoder->EncodeRGB(rgb.data(), width, height, quality, jpeg);
                if (ok) {
                    processed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    producer.join();
    for (auto& t : workers) {
        t.join();
    }

    const auto finish = std::chrono::steady_clock::now();
    const double elapsedSec = std::chrono::duration<double>(finish - start).count();

    Result r;
    r.workerCount = workerCount;
    r.produced = produced.load(std::memory_order_relaxed);
    r.dropped = dropped.load(std::memory_order_relaxed);
    r.processed = processed.load(std::memory_order_relaxed);
    r.elapsedSec = elapsedSec;
    r.inputFps = r.produced / std::max(1e-9, elapsedSec);
    r.outputFps = r.processed / std::max(1e-9, elapsedSec);
    r.dropRate = (r.produced > 0) ? (static_cast<double>(r.dropped) / static_cast<double>(r.produced)) : 0.0;

    return r;
}

int main(int argc, char** argv) {
    int width = 1920;
    int height = 1080;
    int quality = 70;
    int inputFps = 120;
    int durationSec = 8;
    size_t maxQueueSize = 6;

    if (argc > 1) width = std::atoi(argv[1]);
    if (argc > 2) height = std::atoi(argv[2]);
    if (argc > 3) inputFps = std::atoi(argv[3]);
    if (argc > 4) durationSec = std::atoi(argv[4]);
    if (argc > 5) quality = std::atoi(argv[5]);
    if (argc > 6) maxQueueSize = static_cast<size_t>(std::max(1, std::atoi(argv[6])));

    const int beforeWorkers = 1;
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const int afterWorkers = static_cast<int>(std::max(2u, hw));

    std::cout << "Benchmark config: width=" << width
              << " height=" << height
              << " input_fps=" << inputFps
              << " duration_sec=" << durationSec
              << " quality=" << quality
              << " queue=" << maxQueueSize
              << " before_workers=" << beforeWorkers
              << " after_workers=" << afterWorkers
              << "\n";

    const Result before = RunScenario(beforeWorkers, width, height, quality, inputFps, durationSec, maxQueueSize);
    const Result after = RunScenario(afterWorkers, width, height, quality, inputFps, durationSec, maxQueueSize);

    auto printResult = [](const char* tag, const Result& r) {
        std::cout << "[" << tag << "] workers=" << r.workerCount
                  << " produced=" << r.produced
                  << " processed=" << r.processed
                  << " dropped=" << r.dropped
                  << " drop_rate=" << (r.dropRate * 100.0) << "%"
                  << " input_fps=" << r.inputFps
                  << " output_fps=" << r.outputFps
                  << " elapsed=" << r.elapsedSec << "s\n";
    };

    printResult("before(single-worker)", before);
    printResult("after(multi-worker)", after);

    return 0;
}
