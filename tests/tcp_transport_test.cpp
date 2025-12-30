#include "RobotPal/Network/TcpNetworkTransport.h"
#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;
struct Listener {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    Listener() {
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        assert(bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
        socklen_t size = sizeof(address);
        assert(getsockname(fd, reinterpret_cast<sockaddr*>(&address), &size) == 0);
        assert(listen(fd, 1) == 0);
    }
    std::string Url() { return "127.0.0.1:" + std::to_string(ntohs(address.sin_port)); }
    ~Listener() { close(fd); }
};
int main() {
    TcpNetworkTransport transport;
    {
        Listener listener;
        std::vector<uint8_t> frame(1024*1024 + 4, 0x5a);
        const uint32_t length = 1024*1024;
        for (int i=0; i<4; ++i) frame[i] = uint8_t(length >> (8*i));
        const std::vector<uint8_t> next{3,0,0,0,0x11,0x22,0x33};
        auto expected = frame; expected.insert(expected.end(), next.begin(), next.end());
        auto receiver = std::async(std::launch::async, [&] {
            int peer = accept(listener.fd, nullptr, nullptr);
            std::vector<uint8_t> received;
            uint8_t buffer[4096];
            while (received.size() < expected.size()) {
                int n = recv(peer, buffer, sizeof(buffer), 0);
                assert(n > 0); received.insert(received.end(), buffer, buffer+n);
                std::this_thread::sleep_for(1ms); // deliberate slow receiver
            }
            close(peer); assert(received == expected);
        });
        assert(transport.Connect(listener.Url())); transport.Send(frame); transport.Send(next);
        assert(receiver.wait_for(5s) == std::future_status::ready); receiver.get();
        transport.Disconnect(); assert(!transport.IsConnected());
    }
    {
        Listener listener;
        assert(transport.Connect(listener.Url()));
        int peer = accept(listener.fd, nullptr, nullptr); // never read
        std::vector<uint8_t> frame(1024*1024, 0x33);
        for (int i=0; i<200; ++i) {
            transport.Send(frame);
            assert(transport.PendingPackets() <= 6);
            assert(transport.PendingBytes() <= 8*1024*1024);
        }
        assert(transport.DroppedPackets() > 0);
        auto before = std::chrono::steady_clock::now();
        transport.Disconnect();
        assert(std::chrono::steady_clock::now() - before < 2s);
        assert(transport.PendingPackets() == 0); close(peer);
    }
    {
        Listener listener; assert(transport.Connect(listener.Url()));
        int peer = accept(listener.fd, nullptr, nullptr);
        int smallBuffer = 4096;
        setsockopt(peer, SOL_SOCKET, SO_RCVBUF, &smallBuffer, sizeof(smallBuffer));
        transport.Send(std::vector<uint8_t>(8*1024*1024, 0x44));
        auto deadline = std::chrono::steady_clock::now() + 5s;
        while (transport.IsConnected() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(10ms);
        assert(!transport.IsConnected()); // A fully stalled send fails instead of hanging.
        transport.Disconnect(); close(peer);
    }
    // Remote close must stop workers; destruction/reconnect must join them safely.
    {
        Listener listener; assert(transport.Connect(listener.Url()));
        int peer = accept(listener.fd, nullptr, nullptr); close(peer);
        auto deadline = std::chrono::steady_clock::now() + 2s;
        while (transport.IsConnected() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        assert(!transport.IsConnected()); transport.Disconnect();
    }
    std::cout << "TCP loopback slow receiver, bounds, reconnect, disconnect passed\n";
}
