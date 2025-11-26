/**
 * @file IRobotDriver.h
 * @author yoonpyo(cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

class IRobotDriver{
public:
    // 다형성을 가진 기본 클래스에는 반드시 가상 소멸자를 선언하라.
    virtual ~IRobotDriver() = default;

    virtual bool Init() = 0;

    virtual void Drive(float v, float w) = 0;

    virtual void Update(float ts) = 0;
};