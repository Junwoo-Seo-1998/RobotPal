#pragma once
#include "RobotPal/RobotController.h"
#include "RobotPal/Entity.h"

class RealController : public IRobotController
{
public:
    // [변경] NetworkEngine 포인터를 받지 않음 (Entity만 있으면 World 접근 가능)
    RealController(Entity entity);
    virtual ~RealController() = default;

    virtual bool Init() override;
    virtual void Move(const float& v, const float& w) override;
    virtual void Update(const float& dt) override;

private:
    Entity m_Entity;
    
    // 중복 전송 방지용
    float m_LastV = 0.0f;
    float m_LastW = 0.0f;
};