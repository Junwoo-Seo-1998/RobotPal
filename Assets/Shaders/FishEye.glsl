#type vertex
#version 300 es
precision mediump float;

// [Vertex Shader] 화면 전체를 덮는 Quad
layout(location = 0) in vec2 a_Position;
out vec2 v_TexCoords;

void main()
{
    v_TexCoords = a_Position;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type fragment
#version 300 es
precision mediump float;

out vec4 FragColor;
in vec2 v_TexCoords;

uniform samplerCube u_EnvironmentMap;
uniform float u_FOV;    
uniform float u_Aspect; 
uniform float u_Scale; // [추가] 줌 배율 (1.0 = 원형, 1.5 이상 = 꽉 찬 화면)
uniform mat4 u_Rotation;
void main()
{
    // 1. UV 좌표 정규화
    vec2 p = v_TexCoords; 
    p.x *= u_Aspect;

    // 2. 거리(r) 계산
    float r = length(p);
    float phi = atan(p.y, p.x);

    // [삭제] 검은색 테두리 만드는 코드 제거!
    /*
    if (r > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    */

    // 3. 렌즈 매핑 수정 (Zoom 적용)
    // r을 u_Scale로 나누어주면, 더 넓은 화면 영역이 렌즈의 중심부를 확대해서 보여주게 됩니다.
    float theta = (r / u_Scale) * (u_FOV * 0.5);

    // 4. Ray 계산 (기존 동일)
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    
    vec3 rayDir;
    rayDir.x = sinTheta * cos(phi);
    rayDir.y = sinTheta * sin(phi);
    rayDir.z = -cosTheta; 
    
    vec3 worldRayDir = mat3(u_Rotation) * rayDir;

    FragColor = texture(u_EnvironmentMap, worldRayDir);
}