#ifndef __STREAMINGSYSTEMMODULE_H__
#define __STREAMINGSYSTEMMODULE_H__

#include "RobotPal/Network/NetworkEngine.h"
#include "RobotPal/Util/StreamingPipeline.h"
#include <flecs.h>
#include <thread> // [필수] std::thread 멤버 변수를 위해 필요

struct StreamingSystemModule
{
public:
    // 생성자
    StreamingSystemModule(flecs::world &world);
    
    // [필수 추가] 소멸자 선언 (CPP 파일에 구현이 있으므로 선언이 필수입니다)
    ~StreamingSystemModule();

private:
    void RegisterObserver(flecs::world& world);
    void RegisterSystem(flecs::world& world);

    std::unique_ptr<StreamingPipeline> m_worker;
    // [필수 추가] CPP 파일에서 사용 중인 멤버 변수 선언
    flecs::world &m_world;      // CPP에서 m_world->get_info()로 접근하므로 포인터여야 합니다.
    bool m_pboPrimed = false;
    uint32_t m_pendingPboFrameId = 0;
    uint64_t m_pendingPboGeneratedUnixNs = 0;
};

#endif
