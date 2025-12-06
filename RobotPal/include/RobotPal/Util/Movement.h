/**
 * @file Movement.h
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include <glm/glm.hpp>
#include <cmath>

namespace MovementMath {

    // 현재 위치와 회전, 속도를 받아 다음 위치를 계산 (Dead Reckoning)
    inline void CalculateNextStep(
        glm::vec3& pos, 
        glm::vec3& rot, 
        float v, float w, 
        float dt
    ) {
        // 1. 회전 업데이트 (Y축 기준)
        rot.y += w * dt;

        // 2. 위치 업데이트 (바라보는 방향으로 전진)
        // Jetank 기준: Z축이 전방일 경우 cos, X축이 전방일 경우 sin 등 좌표계 확인 필요
        // 현재 코드 기준 (pos.z -= cos, pos.x -= sin) 유지
        pos.x -= std::sin(rot.y) * v * dt;
        pos.z -= std::cos(rot.y) * v * dt;
    }

    // 속도 보간 (감속/가속)
    inline void ApplyFriction(float& v, float& w, float v_decay = 0.93f, float w_decay = 0.95f) {
        v *= v_decay;
        w *= w_decay;
        if (std::abs(v) < 0.001f) v = 0.0f;
        if (std::abs(w) < 0.001f) w = 0.0f;
    }
}