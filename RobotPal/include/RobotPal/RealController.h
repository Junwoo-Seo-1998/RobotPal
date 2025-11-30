/**
 * @file RealController.h
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "RobotPal/RobotController.h"
#include "RobotPal/Entity.h"
#include "RobotPal/NetworkManager.h"
#include <string>

class RealController : public IRobotController
{
public:
    RealController(Entity entity, int port);
    virtual ~RealController() = default;

    // [인터페이스 구현]
    virtual bool Init() override;
    virtual void Move(float v, float w) override;
    virtual void Update(float dt) override;

private:
    // 제어 대상 및 통신 모듈
    Entity m_Entity;
    NetworkManager m_Server;
    int m_Port;

    // 중복 전송 방지용 캐시 (이전 프레임의 명령값)
    float m_LastV = 0.0f;
    float m_LastW = 0.0f;
};