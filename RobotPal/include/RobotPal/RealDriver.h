#pragma once
#include "RobotPal/RobotDriver.h"
#include "RobotPal/TcpServer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class RealDriver : public IRobotDriver
{
public:
    RealDriver(int port);
    virtual ~RealDriver() = default;

    virtual bool Init() override;
    virtual void Drive(float v, float w) override;
    virtual void Update(float dt) override;
    virtual glm::mat4 GetTransform() const override;

private:
    TcpServer m_Server;
    int m_Port;

    // 중복 전송 방지용 캐시
    float m_LastV = 0.0f;
    float m_LastW = 0.0f;

    // 실제 로봇으로부터 수신한 위치 (Visual Sync용)
    glm::vec3 m_RealPos = {0.0f, 0.0f, 0.0f};
    float m_RealYaw = 0.0f;
};