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
#include "RobotPal/Components/Components.h" // Position, Rotation 컴포넌트
#include <cstdio>  // snprintf, sscanf
#include <cmath>   // abs
#include <iostream>

RealController::RealController(Entity entity, int port)
    : m_Entity(entity), m_Port(port)
{
}

bool RealController::Init()
{
    // 1. 엔티티 및 컴포넌트 검사 (SimController와 동일한 스타일)
    if (!m_Entity.IsValid()) {
        std::cout << ">>> [RealController] Entity is invalid!" << std::endl;
        return false;
    }
    // if (!m_Entity.Has<Position>() || !m_Entity.Has<Rotation>()) {
    //     std::cout << ">>> [RealController] Missing required components!" << std::endl;
    //     return false;
    // }

    // 2. 서버 시작 (RealController만의 고유 기능)
    if (!m_Server.Init(m_Port)) {
        std::cout << ">>> [RealController] Failed to start server on port " << m_Port << std::endl;
        return false;
    }
    
    std::cout << ">>> [RealController] Listening on port " << m_Port << "..." << std::endl;
    return true;
}

void RealController::Move(float v, float w)
{
    // 값이 유의미하게 변했을 때만 패킷 전송 (대역폭 최적화)
    if (std::abs(m_LastV - v) > 0.001f || std::abs(m_LastW - w) > 0.001f)
    {
        char buf[64];
        // 프로토콜: CMD:선속도,각속도
        // 예: "CMD:1.50,0.50"
        std::snprintf(buf, sizeof(buf), "CMD:%.2f,%.2f", v, w);
        
        m_Server.Send(std::string(buf));

        // 마지막 값 갱신
        m_LastV = v;
        m_LastW = w;
    }
}

void RealController::Update(float dt)
{
    m_Server.Update();

    if (m_Server.IsConnected())
    {
        std::string msg = m_Server.GetLastReceivedData();
        if (!msg.empty())
        {
            // [수정] test.py 포맷에 맞춰 파싱 (x, y만 존재)
            // 전송 포맷: "STATE:x=1.23,y=4.56"
            float x, z, yaw;
            
            // sscanf_s 대신 표준 sscanf 사용 (호환성)
            if (std::sscanf(msg.c_str(), "STATE:x=%f,y=%f,yaw=%f", &x, &z, &yaw) == 3)
            {
                if (m_Entity.IsValid()) {
                    auto* pos = m_Entity.GetPtr<Position>();
                    auto* rot = m_Entity.GetPtr<Rotation>();
                    
                    if (pos && rot) {
                        // 위치 동기화
                        pos->x = x;
                        pos->z = z; // 2D의 y는 3D의 z로 매핑
                        
                        // [수정] 회전 동기화 (Y축 회전)
                        rot->y = yaw;
                    }
                }
            }
        }
    }
}