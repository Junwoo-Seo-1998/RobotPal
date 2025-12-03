#type vertex
#version 300 es
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoords;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    v_TexCoords = a_TexCoords;
    v_WorldPos = vec3(u_Model * vec4(a_Position, 1.0));
    
    // Non-uniform scaling을 고려한 법선 행렬 계산
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;

    gl_Position = u_Projection * u_View * vec4(v_WorldPos, 1.0);
}

#type fragment
#version 300 es
precision highp float; // SH 및 PBR 계산을 위해 높은 정밀도 권장
out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoords;

// --- [Material Parameters] ---
uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;

// --- [IBL Parameters] ---
uniform vec3 u_ViewPos;
uniform int u_UseIBL;

// 1. Diffuse: SH 계수 (9개)
uniform vec3 u_SH[9]; 

// 2. Specular: Prefiltered Map + BRDF LUT
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDFLUT;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// [핵심] SH 계수를 이용한 Diffuse Irradiance 재구성
// ----------------------------------------------------------------------------
vec3 CalculateIrradianceSH(vec3 n) 
{
    // SH Basis 상수 (Spherical Harmonics Basis Constants)
    const float C1 = 0.429043;
    const float C2 = 0.511664;
    const float C3 = 0.743125;
    const float C4 = 0.886227;
    const float C5 = 0.247708;

    // SH 공식을 사용하여 Irradiance 계산 (텍스처 샘플링 없음 -> 매우 빠름!)
    vec3 irradiance = 
        C4 * u_SH[0] +
        2.0 * C2 * (u_SH[1] * n.y + u_SH[2] * n.z + u_SH[3] * n.x) +
        C1 * u_SH[4] * (n.x * n.y) +
        C1 * u_SH[5] * (n.y * n.z) +
        C3 * u_SH[6] * (3.0 * n.z * n.z - 1.0) +
        C1 * u_SH[7] * (n.x * n.z) +
        C5 * u_SH[8] * (n.x * n.x - n.y * n.y);
    
    return max(irradiance, 0.0); // 음수 방지
}

// ----------------------------------------------------------------------------
// Fresnel Schlick (Roughness 고려)
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------
void main()
{       
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_ViewPos - v_WorldPos);
    vec3 R = reflect(-V, N); 

    // F0 (기본 반사율) 설정: 비금속(0.04) ~ 금속(Albedo)
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, u_Albedo, u_Metallic);

    vec3 ambient = vec3(0.0);

    // --- [IBL Lighting Calculation] ---
    if (u_UseIBL == 1) 
    {
        // 1. Fresnel 항 계산
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, u_Roughness);
        
        // 2. kS(Specular 비율) & kD(Diffuse 비율) 계산
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - u_Metallic; // 금속은 Diffuse가 없음
        
        // 3. Diffuse IBL (SH 사용!)
        vec3 irradiance = CalculateIrradianceSH(N);
        vec3 diffuse    = irradiance * u_Albedo;
        
        // 4. Specular IBL (Prefilter Map + BRDF LUT)
        const float MAX_REFLECTION_LOD = 4.0; // Prefilter Mipmap 레벨 수에 맞춤
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, u_Roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), u_Roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        // 5. 최종 IBL 합성
        ambient = (kD * diffuse + specular) * u_AO;
    }
    else 
    {
        // Fallback (IBL 데이터 없을 때)
        ambient = vec3(0.03) * u_Albedo * u_AO;
    }

    // --- [Direct Lighting] (Optional) ---
    // 여기에 태양광(Directional Light) 등의 계산을 추가하면 됩니다.
    // vec3 Lo = ...;
    // vec3 color = ambient + Lo;
    
    vec3 color = ambient;

    // --- [Post Processing] ---
    // 1. HDR Tone Mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // 2. Gamma Correction
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}