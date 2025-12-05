#include "RobotPal/Network/INetworkTransport.h"
#include "RobotPal/Network/TcpNetworkTransport.h"
#include "RobotPal/Network/WebSocketTransport.h"
#include <memory>

std::shared_ptr<NetworkTransport> NetworkTransport::Create()
{
    #ifndef __EMSCRIPTEN__
        return std::make_shared<TcpNetworkTransport>();
    #else
        return std::make_shared<WebSocketTransport>();
    #endif
}

