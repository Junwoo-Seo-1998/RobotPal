#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <vector>

namespace robotpal {
// Waiting complete frames only: an in-flight TCP frame must never be truncated.
class BoundedSendQueue {
public:
    explicit BoundedSendQueue(size_t count = 6, size_t bytes = 8 * 1024 * 1024)
        : maxCount(count), maxBytes(bytes) {}
    bool Push(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex);
        if (data.empty() || data.size() > maxBytes || maxCount == 0) { ++dropped; return false; }
        while (!queue.empty() && (queue.size() >= maxCount || data.size() > maxBytes - bytes)) {
            bytes -= queue.front().size(); queue.pop_front(); ++dropped;
        }
        queue.push_back(data); bytes += data.size(); return true;
    }
    std::optional<std::vector<uint8_t>> TryPop() {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) return std::nullopt;
        auto data = std::move(queue.front()); queue.pop_front(); bytes -= data.size(); return data;
    }
    void Clear() { std::lock_guard<std::mutex> lock(mutex); queue.clear(); bytes = 0; }
    size_t Size() const { std::lock_guard<std::mutex> lock(mutex); return queue.size(); }
    size_t Bytes() const { std::lock_guard<std::mutex> lock(mutex); return bytes; }
    size_t Dropped() const { std::lock_guard<std::mutex> lock(mutex); return dropped; }
private:
    const size_t maxCount, maxBytes;
    mutable std::mutex mutex;
    std::deque<std::vector<uint8_t>> queue;
    size_t bytes = 0, dropped = 0;
};
// Call under the same lock used to publish the accepted frame to the transport.
class FreshFrameGate {
public:
    bool Accept(uint32_t id) {
        const uint32_t delta = id - last;
        if (initialized && (delta == 0 || delta >= 0x80000000u)) return false;
        initialized = true; last = id; return true;
    }
private:
    bool initialized = false;
    uint32_t last = 0;
};
// Sender returns -2 for EINTR, -1 for errors, 0 for closed connection.
// A positive short write advances the offset without interleaving another packet.
template<class Sender, class Running>
bool SendComplete(const std::vector<uint8_t>& data, Sender send, Running running) {
    size_t offset = 0;
    while (offset < data.size()) {
        if (!running()) return false;
        const size_t count = std::min(data.size() - offset, size_t(std::numeric_limits<int>::max()));
        const int sent = send(data.data() + offset, count);
        if (sent == -2) continue;
        if (sent <= 0 || size_t(sent) > count) return false;
        offset += size_t(sent);
    }
    return true;
}
}
