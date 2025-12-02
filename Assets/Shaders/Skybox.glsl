#type vertex
#version 300 es
layout (location = 0) in vec3 a_Position;

out vec3 localPos;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    localPos = a_Position;

    // 회전값만 적용하기 위해 View 행렬의 이동(Translation) 부분 제거
    mat4 view = mat4(mat3(u_View)); 
    
    vec4 clipPos = u_Projection * view * vec4(localPos, 1.0);

    // [핵심 트릭] 
    // z를 w로 설정하면, Perspective Divide(z/w) 이후 깊이값이 항상 1.0이 됩니다.
    // 즉, 렌더링되는 모든 물체 중 가장 뒤에 그려지게 됩니다.
    gl_Position = clipPos.xyww;
}

#type fragment
#version 300 es
precision mediump float;
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube u_EnvironmentMap;
uniform float u_Intensity;

void main()
{
    vec3 envColor = texture(u_EnvironmentMap, localPos).rgb;
    
    // HDR Tonemapping / Gamma Correction (필요 시 PBR 쉐이더와 동일하게 적용)
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));
    
    FragColor = vec4(envColor * u_Intensity, 1.0);
}