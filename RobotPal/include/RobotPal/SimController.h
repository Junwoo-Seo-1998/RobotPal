/**
 * @file SimController.h
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

class SimController : public IRobotController
{
public:
    SimController(Entity entity);
    virtual ~SimController() = default;

    virtual bool Init() override;
    virtual void Move(float v, float w) override;
    virtual void Update(float dt) override;

private:
    Entity m_Entity;
    
    // 이동 상태
    float m_TargetV = 0.0f;
    float m_TargetW = 0.0f;
    float m_CurrentV = 0.0f;
    float m_CurrentW = 0.0f;
};