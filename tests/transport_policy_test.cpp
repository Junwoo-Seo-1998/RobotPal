#include "RobotPal/Network/StreamingSendPolicy.h"
#include <cassert>
#include <iostream>
using namespace robotpal;
int main() {
    BoundedSendQueue queue(2, 8);
    queue.Push({1,2,3}); queue.Push({4,5,6}); queue.Push({7,8,9});
    assert(queue.Size() == 2 && queue.Bytes() == 6 && queue.Dropped() == 1);
    assert(queue.TryPop()->front() == 4);
    assert(!queue.Push(std::vector<uint8_t>(9)));
    assert(queue.Bytes() == 3);
    BoundedSendQueue byteLimited(10, 8);
    byteLimited.Push({1,1,1}); byteLimited.Push({2,2,2}); byteLimited.Push({3,3,3,3});
    assert(byteLimited.Size() == 2 && byteLimited.Bytes() == 7 && byteLimited.Dropped() == 1);
    assert(byteLimited.TryPop()->front() == 2);
    FreshFrameGate frames;
    assert(frames.Accept(10)); assert(!frames.Accept(9)); assert(!frames.Accept(10));
    assert(frames.Accept(11));
    FreshFrameGate wrap;
    assert(wrap.Accept(0xffffffff)); assert(wrap.Accept(0));
    const std::vector<uint8_t> packet{1,2,3,4,5};
    std::vector<uint8_t> delivered;
    int calls = 0;
    assert(SendComplete(packet, [&](const uint8_t* data, size_t size) {
        if (++calls == 2) return -2; // interrupted system call
        auto n = std::min(size, size_t{2});
        delivered.insert(delivered.end(), data, data+n); return int(n);
    }, [] { return true; }));
    assert(delivered == packet && calls == 4);
    assert(!SendComplete(packet, [](const uint8_t*, size_t) { return 0; }, [] { return true; }));
    assert(!SendComplete(packet, [](const uint8_t*, size_t) { return -1; }, [] { return true; }));
    assert(!SendComplete(packet, [](const uint8_t*, size_t) { return 1; }, [] { return false; }));
    // A stalled consumer cannot increase retained queue memory, and receives newest waiting frames.
    for (int i=0; i<100000; ++i) queue.Push(std::vector<uint8_t>(4, uint8_t(i)));
    assert(queue.Size() == 2 && queue.Bytes() == 8);
    assert(queue.TryPop()->front() == uint8_t(99998));
    assert(queue.TryPop()->front() == uint8_t(99999));
    std::cout << "transport policy regressions passed\n";
}
