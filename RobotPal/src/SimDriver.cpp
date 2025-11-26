/**
 * @file SimDriver.cpp
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "RobotPal/RealDriver.h"
#include <cstdio>
#include <iostream>
#include <cmath>

RealDriver::RealDriver(int port)
    : m_Port(port)
{
}

bool RealDriver::Init()
{
    if (!m_Server.Start(m_Port)) {
        std::cout << ">>> [RealDriver] 서버 시작 실패! Port: " << m_Port << std::endl;
        return false;
    }
    std::cout << ">>> [RealDriver] 서버 대기 중... Port: " << m_Port << std::endl;
    return true;
}

void RealDriver::Drive(float v, float w)
{
    // 값이 유의미하게 변했을 때만 패킷 전송 (대역폭 절약)
    if (std::abs(m_LastV - v) > 0.001f || std::abs(m_LastW - w) > 0.001f)
    {
        char buf[64];
        // 프로토콜: "CMD:v,w" (예: CMD:1.5,0.5)
        snprintf(buf, sizeof(buf), "CMD:%.2f,%.2f", v, w);
        m_Server.Send(std::string(buf));

        m_LastV = v;
        m_LastW = w;
    }
}

void RealDriver::Update(float dt)
{
    // 1. 소켓 이벤트 처리 (필수)
    m_Server.Update();

    // 2. 데이터 수신 (실제 로봇의 상태 동기화)
    if (m_Server.isConnected())
    {
        std::string msg = m_Server.GetLastReceivedData();
        if (!msg.empty())
        {
            // 예: "STATE:x=1.0,y=2.0,yaw=0.5" 형식 파싱
            float x, y, yaw;
            if (sscanf(msg.c_str(), "STATE:x=%f,y=%f,yaw=%f", &x, &y, &yaw) == 3)
            {
                m_RealPos.x = x;
                m_RealPos.z = y; // 3D 공간 매핑 (y -> z)
                m_RealYaw = yaw;
                // std::cout << "[Sync] " << x << ", " << y << std::endl;
            }
        }
    }
}

glm::mat4 RealDriver::GetTransform() const
{
    // 수신된 실제 위치를 바탕으로 렌더링용 행렬 생성
    glm::mat4 tr = glm::translate(glm::mat4(1.0f), m_RealPos);
    return glm::rotate(tr, m_RealYaw, glm::vec3(0.0f, 1.0f, 0.0f));
}