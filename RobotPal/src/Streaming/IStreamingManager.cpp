#include "RobotPal/Streaming/IStreamingManager.h"

// 구체적인 클래스 헤더는 여기서만 포함
#include "RobotPal/Streaming/StreamingManager.h"
// #include "RobotPal/Streaming/WebSocketStreamingManager.h" // 팀원 코드가 있다면 주석 해제

#include <iostream>

std::shared_ptr<IStreamingManager> IStreamingManager::Create(StreamingType type) {
    // 이제 타입 구분 없이 통합된 Manager 반환
    return std::make_shared<StreamingManager>();
}