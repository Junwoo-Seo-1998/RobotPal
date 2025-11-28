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
    if (!m_Server.Init(m_Port)) { // Start -> Init으로 변경됨
        std::cout << ">>> Server Init Failed!" << std::endl;
        return false;
    }
    return true;
}

void RealController::Move(float v, float w)
{
    if (std::abs(m_LastV - v) > 0.001f || std::abs(m_LastW - w) > 0.001f)
    {
        char buf[64];
        // 명령 전송
        std::snprintf(buf, sizeof(buf), "CMD:%.2f,%.2f", v, w);
        m_Server.Send(std::string(buf));
        m_LastV = v;
        m_LastW = w;
    }
}

void RealController::Update(float dt)
{
    m_Server.Update();

    if (m_Server.IsConnected())
    {
        // [변경] 쌓여있는 모든 메시지를 순서대로 처리 (While Loop)
        while (m_Server.HasMessage())
        {
            std::string msg = m_Server.PopMessage();
            
            float x, y, yaw;
            // test.py가 보내는 포맷: "STATE:x=...,y=...,yaw=..."
            if (std::sscanf(msg.c_str(), "STATE:x=%f,y=%f,yaw=%f", &x, &y, &yaw) == 3)
            {
                if (m_Entity.IsValid()) {
                    auto* pos = m_Entity.GetPtr<Position>();
                    auto* rot = m_Entity.GetPtr<Rotation>();
                    
                    if (pos && rot) {
                        pos->x = x;
                        pos->z = y; // 2D y -> 3D z
                        rot->y = yaw;
                    }
                }
            }
        }
    }
}