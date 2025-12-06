/**
 * @file SimController.cpp
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "RobotPal/SimController.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Util/Movement.h"
#include "RobotPal/Entity.h"
SimController::SimController(Entity &entity)
    : m_Entity(entity)
{
}

bool SimController::Init()
{
    std::cout << ">>> [SimController] Initializing..." << std::endl;
    std::cout << ">>> Entity is valid: " << (m_Entity.IsValid() ? "Yes" : "No") << std::endl;
    if (!m_Entity.IsValid()) return false;
    std::cout << ">>> Entity has Position: " << (m_Entity.Has<Position>() ? "Yes" : "No") << std::endl;
    std::cout << ">>> Entity has Rotation: " << (m_Entity.Has<Rotation>() ? "Yes" : "No") << std::endl;
    // if (!m_Entity.Has<Position>() || !m_Entity.Has<Rotation>()) return false;S
    return true;
}

void SimController::Move(const float& v, const float& w)
{
    m_TargetV = v;
    m_TargetW = w;
}

void SimController::Update(const float& dt)
{
    if (!m_Entity.IsValid()) return;
  
    // 1. 보간 (Soft Start/Stop)
    const float accel = 5.0f;
    m_CurrentV += (m_TargetV - m_CurrentV) * accel * dt;
    m_CurrentW += (m_TargetW - m_CurrentW) * accel * dt;

    if (std::abs(m_CurrentV) < 0.01f) m_CurrentV = 0.0f;
    if (std::abs(m_CurrentW) < 0.01f) m_CurrentW = 0.0f;

    // 2. 데이터 수정 (Controller의 본분)
    glm::vec3 pos = m_Entity.GetLocalPosition();
    glm::vec3 rot = m_Entity.GetLocalRotation();


    // (2) 물리 계산 (Dead Reckoning)
    // 회전 (Y축) 업데이트
    MovementMath::CalculateNextStep(pos, rot, m_CurrentV, m_CurrentW, dt);
    MovementMath::ApplyFriction(m_CurrentV, m_CurrentW);

    m_Entity.SetLocalPosition(pos);
    m_Entity.SetLocalRotation(rot);
}