#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

struct ConnectionArg
{
    std::string ip;
    int port;
};


class NetworkTransport
{
public:
    static std::shared_ptr<NetworkTransport> Create();
    
    virtual ~NetworkTransport() = default;

    virtual bool Connect(const std::string& url) = 0;
    virtual void Disconnect() = 0;
    
    // 단순 바이트 전송
    virtual void Send(const std::vector<uint8_t>& data) = 0;
    
    // [추가] 단순 바이트 수신 (Non-blocking 권장 혹은 타임아웃)
    // 리턴값: 수신된 바이트 수, 0이면 연결 종료/대기, -1이면 에러
    // virtual int Receive(uint8_t* buffer, size_t maxSize) = 0;

    virtual bool IsConnected() const = 0;
    //protected:
    //Network
};