#include "RobotPal/Streaming/IStreamingManager.h"

// 구체적인 클래스 헤더는 여기서만 포함
#include "RobotPal/Streaming/TCPStreamingManager.h"
// #include "RobotPal/Streaming/WebSocketStreamingManager.h" // 팀원 코드가 있다면 주석 해제

#include <iostream>

std::shared_ptr<IStreamingManager> IStreamingManager::Create(StreamingType type) {
    switch(type) {
        case StreamingType::WebSocket:
#ifdef __EMSCRIPTEN__
            // return std::make_shared<WebSocketStreamingManager>(); // 팀원 코드
            return nullptr; 
#else
            std::cerr << "[Stream] WebSocket not supported on native build\n";
            return nullptr;
#endif
        case StreamingType::TCP:
#ifndef __EMSCRIPTEN__
            return std::make_shared<TCPStreamingManager>();
#else
            std::cerr << "[Stream] TCP not supported in browser build\n";
            return nullptr;
#endif
        default:
            return nullptr;
    }
}