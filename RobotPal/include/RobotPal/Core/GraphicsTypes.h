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

struct MaterialData {
    std::string name;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    int baseColorTextureIndex = -1; // 텍스처 ID (AssetManager 관리)
    // ... metallic, roughness 등 추가
};

// [2] 서브메쉬 정보 (인덱스 버퍼의 범위)
struct SubMeshInfo {
    uint32_t indexStart;  // 인덱스 배열 시작 위치
    uint32_t indexCount;  // 그릴 인덱스 개수
    int defaultMaterialIndex; // GLTF에 정의된 기본 재질 인덱스
};

// [3] 메쉬 데이터 (유니티의 Mesh 에셋)
struct MeshData {
    int uniqueID=-1;
    std::string name;
    
    // 정점 데이터 (모든 서브메쉬의 정점을 하나로 합침)
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // 서브메쉬 구분자
    std::vector<SubMeshInfo> subMeshes;

    // GPU 핸들 (나중에 업로드 후 채워짐)
    // GPU 핸들 보관 todo
    std::shared_ptr<VertexArray> vao;
};

struct NodeData {
    std::string name;
    
    // 로컬 트랜스폼 (TRS)
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation =  glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    int meshIndex = -1;      // 참조할 MeshData 인덱스
    int parentIndex = -1;    // 부모 노드 인덱스 (루트는 -1)
    std::vector<int> childrenIndices; // 자식 노드 인덱스 리스트
};

// [5] 최종 모델 리소스 컨테이너
struct ModelResource {
    std::vector<MaterialData> materials;
    std::vector<MeshData> meshes;
    std::vector<NodeData> nodes; // 트리를 1차원 배열로 펼침
    
    int rootNodeIndex = 0;
};

//todo: remove later
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

struct Mesh {
    std::shared_ptr<VertexArray> vertexArray;
    std::string name;
    uint32_t vertexCount = 0; // DrawArrays
};



#endif