#include "RobotPal/SandboxScene.h"
#include "RobotPal/Buffer.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Components/Components.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp> // [중요] 쿼터니언 -> 행렬 변환용
#include <imgui.h>
#include <glad/gles2.h>
#include <GLFW/glfw3.h>
#include <iostream>

void SandboxScene::OnEnter()
{
    // 1. 윈도우 리사이즈 이벤트 구독
    m_World.observer<const WindowData>("OnResize")
        .event(flecs::OnSet)
        .each([this](const WindowData &win) {
            std::cout << "화면 크기가 변경됨! " << win.width << "x" << win.height << std::endl;
            m_WindowSize = win; 
        });

    // 2. 모델 로드
    auto modelRes = AssetManager::Get().GetModel("./Assets/jetank.glb");
    if(!modelRes) {
        std::cout << "ERROR: Failed to load model!" << std::endl;
        return;
    }

    // 3. 모든 메쉬에 대해 VAO 생성 (한 번만 수행)
    m_MeshVAOs.clear();
    m_MeshVAOs.resize(modelRes->meshes.size());

    // Vertex 구조체와 일치하는 레이아웃
    BufferLayout layout = {
        {DataType::Float3, "a_Position"},
        {DataType::Float3, "a_Normal"},
        {DataType::Float2, "a_TexCoord"},
        {DataType::Float3, "a_Tangent"},
        {DataType::Int4,   "a_BoneIDs"},
        {DataType::Float4, "a_Weights"}
    };

    for(size_t i = 0; i < modelRes->meshes.size(); ++i) {
        MeshData& meshData = modelRes->meshes[i];
        
        auto va = VertexArray::Create();
        
        // VBO
        auto vb = std::make_shared<VertexBuffer>(
            meshData.vertices.data(), 
            meshData.vertices.size() * sizeof(Vertex)
        );
        vb->SetLayout(layout);
        va->AddVertexBuffer(vb);

        // IBO
        auto ib = IndexBuffer::Create(meshData.indices.data(), meshData.indices.size());
        va->SetIndexBuffer(ib);

        m_MeshVAOs[i] = va; // 저장
    }

    // 4. 테스트용 엔티티 (중심축용)
    m_Center = CreateEntity("center");
    m_Center.SetLocalScale(glm::vec3{1.0f});
    m_CubeColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

    // 5. 쉐이더 생성
    std::string vertexSrc = R"(#version 300 es
        precision mediump float;
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        
        uniform mat4 u_Model;
        uniform mat4 u_View;
        uniform mat4 u_Projection;

        out vec3 v_Normal;
        out vec3 v_FragPos;

        void main()
        {
            v_FragPos = vec3(u_Model * vec4(a_Position, 1.0));
            // 법선 행렬 (Scale 보정)
            v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
            gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
        }
    )";

    std::string fragmentSrc = R"(#version 300 es
        precision mediump float;
        layout(location = 0) out vec4 color;
        
        in vec3 v_Normal;
        in vec3 v_FragPos;

        uniform vec4 u_Color;
        uniform vec3 u_ViewPos;

        void main()
        {
            vec3 lightPos = vec3(5.0, 10.0, 5.0);
            
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * vec3(1.0);

            // Diffuse
            vec3 norm = normalize(v_Normal);
            vec3 lightDir = normalize(lightPos - v_FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0);

            vec3 result = (ambient + diffuse) * u_Color.rgb;
            color = vec4(result, u_Color.a);
        }
    )";

    m_CubeShader = Shader::CreateFromSource("PhongShader", vertexSrc, fragmentSrc);
}

void SandboxScene::OnUpdate(float dt)
{
    if (!m_CubeShader) return;
    auto modelRes = AssetManager::Get().GetModel("./Assets/jetank.glb");
    if (!modelRes || modelRes->nodes.empty()) return;

    // 1. 카메라 행렬 설정
    glm::vec3 viewPos = glm::vec3(0.0f, 0.5f, 1.0f); 
    glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
    
    float aspect = m_WindowSize.GetAspect();
    if(aspect == 0.0f) aspect = 1.77f; // 안전장치
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // 2. 쉐이더 공통 유니폼 설정
    m_CubeShader->Bind();
    m_CubeShader->SetMat4("u_View", view);
    m_CubeShader->SetMat4("u_Projection", projection);
    m_CubeShader->SetFloat4("u_Color", m_CubeColor);
    m_CubeShader->SetFloat3("u_ViewPos", viewPos);

    // 3. 모델 전체 회전 (World Matrix)
    // 시간이 지남에 따라 Y축 회전
    glm::mat4 rootTransform = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0, 1, 0));

    // 4. 루트 노드부터 재귀적 렌더링 시작
    RenderNode(modelRes->nodes[modelRes->rootNodeIndex], rootTransform, *modelRes);
}

void SandboxScene::RenderNode(const NodeData& node, const glm::mat4& parentTransform, const ModelResource& modelRes)
{
    // A. Local Transform 계산 (TRS)
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), node.translation);
    glm::mat4 rotate = glm::mat4_cast(glm::quat(node.rotation));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), node.scale);
    
    glm::mat4 localTransform = translate * rotate * scale;

    // B. Global Transform 계산 (Parent * Local)
    glm::mat4 globalTransform = parentTransform * localTransform;

    // C. 메쉬 그리기 (현재 노드가 메쉬를 가지고 있다면)
    if (node.meshIndex >= 0 && node.meshIndex < m_MeshVAOs.size()) {
        
        m_CubeShader->SetMat4("u_Model", globalTransform);

        // 해당 메쉬의 VAO 바인딩
        auto& va = m_MeshVAOs[node.meshIndex];
        va->Bind();

        // 서브메쉬 단위로 그리기
        const auto& meshData = modelRes.meshes[node.meshIndex];
        for (const auto& sub : meshData.subMeshes) {
            glDrawElements(GL_TRIANGLES, 
                           sub.indexCount, 
                           GL_UNSIGNED_INT, 
                           (void*)(sub.indexStart * sizeof(uint32_t)));
        }
    }

    // D. 자식 노드 순회 (현재 계산된 행렬을 부모 행렬로 넘김)
    for (int childIdx : node.childrenIndices) {
        RenderNode(modelRes.nodes[childIdx], globalTransform, modelRes);
    }
}

void SandboxScene::OnExit()
{
    m_MeshVAOs.clear();
    m_CubeShader = nullptr;
}

void SandboxScene::OnImGuiRender()
{
    ImGui::Begin("Settings");
    ImGui::ColorEdit4("Model Color", &m_CubeColor.x);
    ImGui::Text("Mesh Count: %d", (int)m_MeshVAOs.size());
    ImGui::End();
}