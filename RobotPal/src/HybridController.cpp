/**
 * @file HybridController.cpp
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "RobotPal/HybridController.h"
#include <iostream>

HybridController::HybridController(Entity &Target, int port)
{
    // 1. 시뮬레이션 컨트롤러는 '메인 로봇'을 움직임 (즉각 반응)
    m_Sim = std::make_unique<SimController>(Target);

    // 2. 리얼 컨트롤러는 '고스트 로봇'을 움직임 (실제 데이터 반영)
    // 만약 고스트 로봇을 안 쓸 거면 ghostEntity에 빈 Entity()를 넘기면 됨
    m_Real = std::make_unique<RealController>(Target, port);
}

bool HybridController::Init()
{
    bool simOk = m_Sim->Init();
    bool realOk = m_Real->Init();

    if (!realOk) {
        std::cout << ">>> [Hybrid] Real Controller Init Failed (Network Error?)" << std::endl;
        // 네트워크 실패해도 시뮬레이션은 되게 할지, 멈출지 결정 (여기선 경고만)
    }
    return simOk; // 시뮬레이션만 돼도 일단 성공으로 간주
}

void HybridController::Move(const float& v, const float& w)
{
    // 양쪽 모두에게 명령 전달
    std::cout << ">>> [Hybrid] Move Command - v: " << v << ", w: " << w << std::endl;
    m_Sim->Move(v, w);  // 화면상 로봇 움직임 예측
    m_Real->Move(v, w); // 실제 로봇에게 패킷 전송
}

void HybridController::Update(const float& dt)
{
    m_Sim->Update(dt);  // 물리 연산 수행
    m_Real->Update(dt); // 네트워크 수신 및 고스트 엔티티 동기화
}