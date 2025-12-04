
#pragma once
#include <string>
#include <vector>
#include <cstdint>

class INetworkTransport
{
public:
    virtual ~INetworkTransport() = default;

    virtual bool Connect(const std::string& url) = 0;
    virtual void Disconnect() = 0;
    virtual void Send(const std::vector<uint8_t>& data) = 0;
    virtual bool IsConnected() const = 0;
};