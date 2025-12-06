#ifndef __GLOBALCOMPONENT_H__
#define __GLOBALCOMPONENT_H__
#include "RobotPal/Core/ResourceID.h"
struct WindowData {
    float width=1.0f, height=1.0f;
    float GetAspect() const { return width / height; }
};

struct Skybox {
    ResourceID textureID; // 로드된 HDR 텍스처 또는 큐브맵 ID
    float intensity = 1.0f; // 밝기 조절
    float rotation = 0.0f;  // 회전
};

// [Output] 베이킹된 조명 데이터 (Singleton으로 사용)
struct GlobalLighting {
    // 1. Diffuse (SH): GLES 3.0 대역폭 최적화의 핵심
    // 큐브맵 텍스처 대신 9개의 vec3 계수만 사용하여 조명 계산
    glm::vec3 shCoeffs[9]; 
    ResourceID environmentMap;
    // 2. Specular (Environment Reflections)
    // 거칠기(Roughness) 레벨별로 Convolution된 큐브맵 (Mipmap 사용)
    ResourceID prefilteredMap; 
    
    // 3. PBR Lookup Table (Split-Sum Approximation)
    // 모든 물체가 공유하는 2D LUT 텍스처
    ResourceID brdfLUT; 

    float intensity = 1.0f;
};
#endif
