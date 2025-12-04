#pragma once
#include <memory>
#include "RobotPal/Network/INetworkTransport.h"
#include "RobotPal/Network/TcpNetworkTransport.h"
#include "RobotPal/Network/WebSocketTransport.h" // 새로 추가될 헤더

class TransportFactory
{
public:
    static std::unique_ptr<INetworkTransport> Create()
    {
#ifdef __EMSCRIPTEN__
        return std::make_unique<WebSocketTransport>();
#else
        return std::make_unique<TcpNetworkTransport>();
#endif
    }
};