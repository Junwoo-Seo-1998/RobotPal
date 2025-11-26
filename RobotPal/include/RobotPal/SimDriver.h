/**
 * @file SimDriver.h
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "RobotPal/RobotDriver.h"
#include "RobotPal/Entity.h"
#include <glm/glm.hpp>

class SimDriver : public IRobotDriver
{
public:
    // 생성자에서 제어할 엔티티를 받습니다.
    SimDriver(Entity entity);
    virtual ~SimDriver() = default;

    virtual bool Init() override;
    virtual void Drive(float v, float w) override;
    virtual void Update(float dt) override;
    virtual glm::mat4 GetTransform() const override;

private:
    Entity m_Entity;

    // 목표 속도
    float m_TargetV = 0.0f;
    float m_TargetW = 0.0f;

    // 현재 속도 (가속/감속 효과를 위한 보간용)
    float m_CurrentV = 0.0f;
    float m_CurrentW = 0.0f;

    // 마지막으로 계산된 변환 행렬 (GetTransform용)
    glm::mat4 m_CachedTransform = glm::mat4(1.0f);
};