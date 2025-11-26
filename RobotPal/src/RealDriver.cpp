#include "RobotPal/RealController.h"
#include "RobotPal/Components/Components.h"
#include <cstdio>
#include <iostream>
#include <cmath>

RealController::RealController(Entity entity, int port)
    : m_Entity(entity), m_Port(port)
{
}

bool RealController::Init()
{
    if (!m_Server.Start(m_Port)) {
        std::cout << ">>> [RealController] Server Start Failed!" << std::endl;
        return false;
    }
    return true;
}

void RealController::Move(float v, float w)
{
    // 중복 전송 방지
    if (std::abs(m_LastV - v) > 0.001f || std::abs(m_LastW - w) > 0.001f)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "CMD:%.2f,%.2f", v, w);
        m_Server.Send(std::string(buf));
        m_LastV = v;
        m_LastW = w;
    }
}

void RealController::Update(float dt)
{
    m_Server.Update();

    if (m_Server.isConnected())
    {
        std::string msg = m_Server.GetLastReceivedData();
        if (!msg.empty())
        {
            float x, z, yaw;
            if (sscanf(msg.c_str(), "STATE:x=%f,y=%f,yaw=%f", &x, &z, &yaw) == 3)
            {
                // 데이터 동기화 (Visual Sync)
                if (m_Entity.IsValid()) {
                    auto* pos = m_Entity.GetPtr<Position>();
                    auto* rot = m_Entity.GetPtr<Rotation>();
                    
                    if (pos && rot) {
                        pos->x = x;
                        pos->z = z;
                        rot->y = yaw;
                    }
                }
            }
        }
    }
}