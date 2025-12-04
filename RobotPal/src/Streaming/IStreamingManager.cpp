#include "RobotPal/Streaming/IStreamingManager.h"
#include "RobotPal/Streaming/StreamingManager.h" // 통합된 매니저

// 파라미터 없이 바로 생성
std::shared_ptr<IStreamingManager> IStreamingManager::Create(flecs::world& world) {
    return std::make_shared<StreamingManager>(world);
}