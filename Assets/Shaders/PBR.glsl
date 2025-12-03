#type vertex
#version 300 es
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec4 a_Tangent; // [추가] glTF는 Tangent.w에 Bitangent 방향 부호를 저장함

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoords;
out mat3 v_TBN; // [추가] Fragment Shader로 보낼 TBN 행렬

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    v_TexCoords = vec2(a_TexCoords.x, 1.0 - a_TexCoords.y);
    //v_TexCoords = a_TexCoords;
    v_WorldPos = vec3(u_Model * vec4(a_Position, 1.0));
    
    // Normal Matrix
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    v_Normal = normalize(normalMatrix * a_Normal); // Vertex Normal (Fallback용)

    // [추가] TBN Matrix 계산 (Normal Mapping용)
    vec3 T = normalize(normalMatrix * a_Tangent.xyz);
    vec3 N = normalize(normalMatrix * a_Normal);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt process (직교화)
    vec3 B = cross(N, T) * a_Tangent.w; // Bitangent 계산 (Handedness 고려)
    
    v_TBN = mat3(T, B, N);

    gl_Position = u_Projection * u_View * vec4(v_WorldPos, 1.0);
}

#type fragment
#version 300 es
precision highp float;
out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoords;
in mat3 v_TBN; // [추가]

// --- [Material Parameters] ---
// 기본값(Factor)은 텍스처와 곱해집니다 (glTF 표준)
uniform vec4 u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform vec3 u_EmissiveFactor;

// --- [Textures & Flags] ---
uniform sampler2D u_BaseColorTexture;
uniform int u_HasBaseColorTex;

uniform sampler2D u_MetallicRoughnessTexture; // G: Roughness, B: Metallic
uniform int u_HasMetallicRoughnessTex;

uniform sampler2D u_NormalTexture;
uniform int u_HasNormalTex;

uniform sampler2D u_OcclusionTexture; // R: Occlusion
uniform int u_HasOcclusionTex;

uniform sampler2D u_EmissiveTexture;
uniform int u_HasEmissiveTex;

// --- [IBL Parameters] ---
uniform vec3 u_ViewPos;
uniform int u_UseIBL;
uniform vec3 u_SH[9]; 
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
    // 1. Base Color (Albedo) 처리
    vec4 baseColor = u_BaseColorFactor;
    if (u_HasBaseColorTex == 1) 
    {
        vec4 texColor = texture(u_BaseColorTexture, v_TexCoords);
        
        // [중요] 텍스처가 sRGB라면 Linear 공간으로 변환 (Gamma 2.2)
        // 주의: Alpha(투명도) 값은 이미 Linear이므로 변환하면 안 됩니다. 오직 .rgb만 변환합니다.
        texColor.rgb = pow(texColor.rgb, vec3(2.2)); 
        
        baseColor *= texColor; 
    }
    vec3 albedo = baseColor.rgb;
    float alpha = baseColor.a; // Alpha Test가 필요하면 여기서 discard

    // 2. Metallic & Roughness 처리
    float metallic = u_MetallicFactor;
    float roughness = u_RoughnessFactor;
    
    if (u_HasMetallicRoughnessTex == 1) {
        // glTF: B = Metallic, G = Roughness
        vec4 mrSample = texture(u_MetallicRoughnessTexture, v_TexCoords);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }

    // 3. Normal Mapping 처리
    vec3 N;
    if (u_HasNormalTex == 1) {
        vec3 normalSample = texture(u_NormalTexture, v_TexCoords).rgb;
        normalSample = normalSample * 2.0 - 1.0; // [0,1] -> [-1,1]
        N = normalize(v_TBN * normalSample);     // Tangent -> World Space
    } else {
        N = normalize(v_Normal); // 텍스처 없으면 Vertex Normal 사용
    }

    // 4. Occlusion (AO) 처리
    float ao = 1.0;
    if (u_HasOcclusionTex == 1) {
        ao = texture(u_OcclusionTexture, v_TexCoords).r; // R channel
    }

    // 5. Emissive 처리
    vec3 emissive = u_EmissiveFactor;
    if (u_HasEmissiveTex == 1) {
        vec3 emissiveSample = texture(u_EmissiveTexture, v_TexCoords).rgb;
        
        // 중요: Emissive도 색상이므로 sRGB -> Linear 변환 필요
        emissiveSample = pow(emissiveSample, vec3(2.2));
        
        emissive *= emissiveSample;
    }

    vec3 V = normalize(u_ViewPos - v_WorldPos);
    vec3 R = reflect(-V, N); 

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 ambient = vec3(0.0);

    // --- [IBL Lighting Calculation] ---
    if (u_UseIBL == 1) 
    {
        // 1. Fresnel 항 계산
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        
        // 2. kS(Specular 비율) & kD(Diffuse 비율) 계산
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic; // 금속은 Diffuse가 없음
        
        // 3. Diffuse IBL (SH 사용!)
        vec3 irradiance = CalculateIrradianceSH(N);
        vec3 diffuse    = irradiance * albedo;
        
        // 4. Specular IBL (Prefilter Map + BRDF LUT)
        const float MAX_REFLECTION_LOD = 4.0; // Prefilter Mipmap 레벨 수에 맞춤
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        // 5. 최종 IBL 합성
        ambient = (kD * diffuse + specular) * ao;
    }
    else 
    {
        // Fallback (IBL 데이터 없을 때)
        ambient = vec3(0.03) * albedo * ao;
    }

    // --- [Direct Lighting] (Optional) ---
    // 여기에 태양광(Directional Light) 등의 계산을 추가하면 됩니다.
    // vec3 Lo = ...;
    // vec3 color = ambient + Lo;
    
    vec3 color = ambient + emissive;

    // --- [Post Processing] ---
    // 1. HDR Tone Mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // 2. Gamma Correction
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}