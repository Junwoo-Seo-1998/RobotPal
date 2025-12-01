/**
 * @file RealController.cpp
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "RobotPal/RealController.h"
#include "RobotPal/Util/Movement.h" // 분리한 로직 포함
#include "RobotPal/Components/Components.h"
#include <cstdio>
#include <iostream>
#include <cmath>


const std::string ip = "127.0.0.1";// 추후 중계 서버의 IP로 변경
RealController::RealController(Entity &entity, int port)
    : m_Entity(entity), m_Port(port)
{
}

bool RealController::Init()
{
    if (!m_Server.Init(ip,m_Port)) {
        std::cout << ">>> Server Init Failed!" << std::endl;
        return false;
    }
    return true;
}

void RealController::Move(const float& v, const float& w)
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



void RealController::Update(const float& dt)
{
    m_Server.Update();

    if (!m_Entity.IsValid()) return;

    bool packetReceived = false;

    // 1. 네트워크 데이터 처리
    if (m_Server.IsConnected())
    {
        while (m_Server.HasMessage())
        {
            std::string msg = m_Server.PopMessage();
            if (msg.find("STATE:") != 0) continue;

            float x, y, yaw;
            // test.py 포맷: STATE:x=...,y=...,yaw=...
            if (std::sscanf(msg.c_str(), "STATE:x=%f,y=%f,yaw=%f", &x, &y, &yaw) == 3)
            {
                // [데이터 동기화] 실제 로봇 위치로 강제 이동 (보정)
                // 좌표계 변환 필요 (2D x,y -> 3D x,z)
                m_Entity.SetLocalPosition({x, 0.0f, y});
                m_Entity.SetLocalRotation({0.0f, yaw, 0.0f});
                
                packetReceived = true;
            }
        }
    }

    // 2. 패킷이 안 왔을 때의 예측 이동 (Dead Reckoning)
    // 네트워크가 끊기거나 지연될 때 부드럽게 움직이게 함
    if (!packetReceived)
    {
        // (1) 현재 상태 가져오기
        glm::vec3 pos = m_Entity.GetLocalPosition();
        glm::vec3 rot = m_Entity.GetLocalRotation();

        // (2) 속도 감쇠 (명령이 없으면 서서히 멈춤)

        // (3) 이동 계산 (공통 로직 사용)

        // (4) 적용
        m_Entity.SetLocalPosition(pos);
        m_Entity.SetLocalRotation(rot);
    }
}