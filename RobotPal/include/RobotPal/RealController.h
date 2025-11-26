#pragma once
#include "RobotPal/RobotController.h"
#include "RobotPal/TcpServer.h"
#include "RobotPal/Entity.h"
#include <string>

class RealController : public IRobotController
{
public:
    RealController(Entity entity, int port);
    virtual ~RealController() = default;

    virtual bool Init() override;
    virtual void Move(float v, float w) override;
    virtual void Update(float dt) override;

private:
    TcpServer m_Server;
    Entity m_Entity;
    int m_Port;

    float m_LastV = 0.0f;
    float m_LastW = 0.0f;
};