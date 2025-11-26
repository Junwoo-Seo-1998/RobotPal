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
#include <glm/glm.hpp>

/**
 * @brief 로봇 제어 인터페이스
 * 
 */
class IRobotDriver {
public:
    virtual ~IRobotDriver() = default;

    // 초기화
    virtual bool Init() = 0;
    
    // 속도 명령 (선속도 v, 각속도 w)
    virtual void Drive(float v, float w) = 0;
    
    // 매 프레임 업데이트 (시간 dt)
    virtual void Update(float dt) = 0;

    // [SimDriver용] 렌더링을 위한 루트 변환 행렬 가져오기
    // NetworkDriver인 경우 그냥 단위 행렬이나 마지막 추정 위치 반환
    virtual glm::mat4 GetTransform() const = 0;
};