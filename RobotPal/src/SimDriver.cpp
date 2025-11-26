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

#include "RobotPal/RobotDriver.h"
#include "RobotPal/Entity.h"
#include "RobotPal/GlobalComponents.h"

class SimDriver: public IRobotDriver{
public:
    SimDriver(Entity entity) : m_Entity(entity){}
    bool Init() override {return true;}
    void Drive(float v, float w) override{
        m_TargetV = v;
        m_TargetW = w;
    }

    void Update(float ts) override{
        // 구현
    }

private:
    Entity m_Entity;
    float m_TargetV = 0.0f, m_TargetW = 0.0f;
};