#pragma once
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <mutex>

namespace robotpal::benchmark {
inline uint64_t SteadyNowNs() { return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }
inline uint64_t UnixNowNs() { return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); }

class EventLog {
public:
    static EventLog& Instance() { static EventLog log; return log; }
    bool Enabled() const { return m_enabled; }
    void Write(const char* event, uint64_t frameId = 0, uint64_t durationNs = 0, uint64_t value = 0, const char* reason = "") {
        if (!m_enabled) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stream << "{\"event\":\"" << event << "\",\"frame_id\":" << frameId
                 << ",\"steady_ns\":" << SteadyNowNs() << ",\"unix_ns\":" << UnixNowNs()
                 << ",\"duration_ns\":" << durationNs << ",\"value\":" << value
                 << ",\"reason\":\"" << reason << "\"}\n";
        m_stream.flush();
    }
private:
    EventLog() { const char* path = std::getenv("ROBOTPAL_BENCHMARK_LOG"); if (path && *path) { m_stream.open(path, std::ios::out | std::ios::trunc); m_enabled = m_stream.good(); } }
    bool m_enabled = false;
    std::ofstream m_stream;
    std::mutex m_mutex;
};
inline void Event(const char* name, uint64_t frameId = 0, uint64_t durationNs = 0, uint64_t value = 0, const char* reason = "") { EventLog::Instance().Write(name, frameId, durationNs, value, reason); }
}
