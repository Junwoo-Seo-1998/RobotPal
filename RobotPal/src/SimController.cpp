#include "RobotPal/SimController.h"
#include "RobotPal/Components/Components.h"

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
    m_CurrentV += (m_TargetV - m_CurrentV) * dt;
    m_CurrentW += (m_TargetW - m_CurrentW) * dt;

    if (std::abs(m_CurrentV) < 0.01f) m_CurrentV = 0.0f;
    if (std::abs(m_CurrentW) < 0.01f) m_CurrentW = 0.0f;

    // 2. 데이터 수정 (Controller의 본분)
    glm::vec3 pos = m_Entity.GetLocalPosition();
    glm::vec3 rot = m_Entity.GetLocalRotation();
    
    // (2) 물리 계산 (Dead Reckoning)
    // 회전 (Y축) 업데이트
    rot.y += m_CurrentW * dt;


    pos.z += std::sin(rot.y) * m_CurrentV * dt;
    pos.x -= std::cos(rot.y) * m_CurrentV * dt;

    m_CurrentV*=0.93f;
    m_CurrentW*=0.95f;
    m_Entity.SetLocalPosition(pos);
    m_Entity.SetLocalRotation(rot);


}