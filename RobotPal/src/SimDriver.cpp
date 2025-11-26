#include "RobotPal/SimController.h"
#include "RobotPal/Components/Components.h"
#include <cmath>

SimController::SimController(Entity entity)
    : m_Entity(entity)
{
}

bool SimController::Init()
{
    if (!m_Entity.IsValid()) return false;
    if (!m_Entity.Has<Position>() || !m_Entity.Has<Rotation>()) return false;
    return true;
}

void SimController::Move(float v, float w)
{
    m_TargetV = v;
    m_TargetW = w;
}

void SimController::Update(float dt)
{
    if (!m_Entity.IsValid()) return;

    // 1. 보간 (Soft Start/Stop)
    const float accel = 5.0f;
    m_CurrentV += (m_TargetV - m_CurrentV) * accel * dt;
    m_CurrentW += (m_TargetW - m_CurrentW) * accel * dt;

    if (std::abs(m_CurrentV) < 0.01f) m_CurrentV = 0.0f;
    if (std::abs(m_CurrentW) < 0.01f) m_CurrentW = 0.0f;

    // 2. 데이터 수정 (Controller의 본분)
    auto* pos = m_Entity.GetPtr<Position>();
    auto* rot = m_Entity.GetPtr<Rotation>();

    if (pos && rot)
    {
        rot->y += m_CurrentW * dt;
        pos->x += std::sin(rot->y) * m_CurrentV * dt;
        pos->z += std::cos(rot->y) * m_CurrentV * dt;
    }
}