#pragma once
#include "RobotPal/Streaming/IStreamingManager.h"
#include "stb_image_write.h"

#ifndef __EMSCRIPTEN__

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include <vector>
#include <cstdint>
#include <memory>
#include <iostream>

class TCPStreamingManager : public IStreamingManager
{
public:
    TCPStreamingManager();
    virtual ~TCPStreamingManager();

    void Init() override;
    void Shutdown() override;

    void ConnectToServer(const std::string& url) override;
    void Disconnect() override;

    void SendFrame(const FrameData& frame) override;

private:
    int m_sock;

    struct WriteContext {
        std::vector<uint8_t> buffer;
    };

    static void write_func(void* ctx, void* data, int size);
};

#endif // !__EMSCRIPTEN__
