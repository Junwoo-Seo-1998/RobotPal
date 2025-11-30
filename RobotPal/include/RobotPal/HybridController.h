/**
 * @file HybridController.h
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
#include "RobotPal/SimController.h"
#include "RobotPal/RealController.h"
#include <memory>

class HybridController : public IRobotController
{
public:
    // mainEntity: 화면에 보여질 메인 로봇 (Sim이 제어)
    // ghostEntity: 실제 로봇의 위치를 보여줄 그림자 로봇 (Real이 제어, 선택사항)
    HybridController(Entity &target, int port);
    virtual ~HybridController() = default;

    virtual bool Init() override;
    virtual void Move(const float& v, const float& w) override;
    virtual void Update(const float& dt) override;

private:
    std::unique_ptr<SimController> m_Sim;
    std::unique_ptr<RealController> m_Real;
};