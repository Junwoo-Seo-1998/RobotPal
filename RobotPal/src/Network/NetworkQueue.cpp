#include "RobotPal/Network/NetworkQueue.h"

void NetworkQueue::Push(const std::vector<uint8_t> &rawData)
{
    std::lock_guard<std::mutex> lock(mtx);
    packetQueue.push({rawData});
}

void NetworkQueue::Push(const uint8_t *data, size_t length)
{
    std::lock_guard<std::mutex> lock(mtx);
    packetQueue.push({std::vector<uint8_t>(data, data + length)});
}

std::optional<Packet> NetworkQueue::TryPop()
{
    std::lock_guard<std::mutex> lock(mtx);

    if (packetQueue.empty())
    {
        return std::nullopt;
    }

    Packet packet = std::move(packetQueue.front());
    packetQueue.pop();

    return packet;
}

bool NetworkQueue::Empty()
{
    std::lock_guard<std::mutex> lock(mtx);
    return packetQueue.empty();
}

void NetworkQueue::Clear()
{
    std::lock_guard<std::mutex> lock(mtx);
    std::queue<Packet> empty;
    std::swap(packetQueue, empty);
}
