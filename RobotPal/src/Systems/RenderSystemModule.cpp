#include "RobotPal/Systems/RenderSystemModule.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Core/AssetManager.h"
#include <glad/gles2.h>
RenderSystemModule::RenderSystemModule(flecs::world &world)
{
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

    m_SimpleShader = Shader::CreateFromSource("PhongShader", vertexSrc, fragmentSrc);

    world.module<RenderSystemModule>();


    // 1. 윈도우 리사이즈 이벤트 구독
    world.observer<const WindowData>("OnResize")
        .event(flecs::OnSet)
        .each([this](const WindowData &win)
        {
            //std::cout << "화면 크기가 변경됨! " << win.width << "x" << win.height << std::endl;
            m_WindowSize = win; 
        });


    RegisterSystem(world);


}

void RenderSystemModule::RegisterSystem(flecs::world &world)
{
    world.system<const MeshFilter, const MeshRenderer, const TransformMatrix>("RenderSystem")
    .kind(flecs::OnStore)
    .term_at(2).second<World>()
    .each([&](flecs::entity entity, const MeshFilter& mf, const MeshRenderer& mr, const TransformMatrix& tm)
    {
        // 1. 카메라 행렬 설정
        glm::vec3 viewPos = glm::vec3(0.0f, 0.5f, 1.0f);
        glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        float aspect = m_WindowSize.GetAspect();
        if (aspect == 0.0f)
            aspect = 1.77f; // 안전장치
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        // 2. 쉐이더 공통 유니폼 설정
        m_SimpleShader->Bind();
        m_SimpleShader->SetMat4("u_View", view);
        m_SimpleShader->SetMat4("u_Projection", projection);
        m_SimpleShader->SetFloat4("u_Color", {0.8f, 0.8f, 0.8f, 1.f});
        m_SimpleShader->SetFloat3("u_ViewPos", viewPos);

        // C. 메쉬 그리기
        {
            m_SimpleShader->SetMat4("u_Model", tm);

            // 해당 메쉬의 VAO 바인딩
            auto meshData = AssetManager::Get().GetMesh(mf.meshID);
            meshData->vao->Bind();

            // 서브메쉬 단위로 그리기
            for (const auto &sub : meshData->subMeshes)
            {
                glDrawElements(GL_TRIANGLES,
                            sub.indexCount,
                            GL_UNSIGNED_INT,
                            (void *)(sub.indexStart * sizeof(uint32_t)));
            }
        }
    });
}