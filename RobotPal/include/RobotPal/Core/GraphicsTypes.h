#ifndef __GRAPHICSTYPES_H__
#define __GRAPHICSTYPES_H__
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "RobotPal/VertexArray.h"
#include "RobotPal/Shader.h"

struct Vertex {
    // 1. 필수 데이터
    glm::vec3 Position;  // 위치 (x, y, z)
    glm::vec3 Normal;    // 법선 (조명 계산용)
    glm::vec2 TexCoords; // UV 좌표 (텍스처 매핑용)

    // 2. 노멀 매핑용 (필수급)
    glm::vec3 Tangent;   // 접선 (Normal Map 계산에 필수)
    // (Bitangent는 셰이더에서 Cross Product로 계산하므로 저장 안 해도 됨)

    // 3. 애니메이션용 (Skinned Mesh) - 선택 사항이지만 엔진이라면 넣는 게 좋음
    glm::ivec4 BoneIDs;  // 뼈 인덱스 (최대 4개 뼈 영향)
    glm::vec4 Weights;   // 가중치 (0.0 ~ 1.0)
    
    // 초기화 생성자
    Vertex() {
        Position = glm::vec3(0);
        Normal = glm::vec3(0, 1, 0);
        TexCoords = glm::vec2(0);
        Tangent = glm::vec3(0);
        BoneIDs = glm::ivec4(-1); // 뼈 없음
        Weights = glm::vec4(0);
    }
};

struct PrimitiveData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int materialIndex;
};

struct Mesh {
    std::shared_ptr<VertexArray> vertexArray;
    std::string name;
    uint32_t vertexCount = 0; // DrawArrays
};

struct Material {
    std::shared_ptr<Shader> shader;
    //std::shared_ptr<Texture> mainTexture;
    glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    
    // void Bind() const {
    //     if(shader) {
    //         shader->Bind();
    //         shader->SetFloat4("u_Color", baseColor);
    //         if (mainTexture) {
    //             // shader->SetInt("u_MainTex", 0); 
    //             // mainTexture->Bind(0);
    //         }
    //     }
    // }
};

struct MeshFilter {
    std::shared_ptr<Mesh> mesh; 
};

struct MeshRenderer {
    std::shared_ptr<Material> material;
    // bool castShadows = true;
};

#endif