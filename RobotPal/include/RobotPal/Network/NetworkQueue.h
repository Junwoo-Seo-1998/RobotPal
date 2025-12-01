#ifndef __NETWORKQUEUE_H__
#define __NETWORKQUEUE_H__
#include <queue>
#include <mutex>
#include <vector>
#include <optional>

// 모든 패킷의 기본 단위 (바이너리 데이터)
struct Packet {
    std::vector<uint8_t> data;
};

class NetworkQueue {
public:
    // [생산자 - Network Thread]
    void Push(const std::vector<uint8_t>& rawData);

    void Push(const uint8_t* data, size_t length);

    // [소비자 - Main Game Thread]
    // 큐에서 데이터를 하나 꺼냅니다. 없으면 std::nullopt 반환
    std::optional<Packet> TryPop();

    // [소비자] 큐가 비었는지 확인
    bool Empty();

    // [관리] 큐 비우기 (씬 전환 시 등)
    void Clear();

private:
    std::queue<Packet> packetQueue;
    std::mutex mtx;
};
#endif