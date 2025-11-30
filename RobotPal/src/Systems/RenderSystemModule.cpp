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
            m_WindowSize = win; });

    RegisterSystem(world);
}


glm::mat4 CreateViewMatrixFromWorld(const glm::mat4& worldMatrix) {
    // 1. 위치(Position) 추출 (4열)
    glm::vec3 pos =  glm::vec3(worldMatrix*glm::vec4(0.f, 0.f, 0.f, 1.f));

    // 2. Forward(앞) 벡터 추출 및 정규화
    // OpenGL 메모리 레이아웃상 3열(인덱스 2)은 로컬 Z축(Backward)입니다.
    // 카메라는 -Z를 보므로, 이를 반전시켜 Forward를 구합니다.
    glm::vec3 backward = glm::vec3(worldMatrix[2]); 
    glm::vec3 forward = glm::normalize(-backward); // 스케일 제거됨

    // 3. Right(오른쪽) 벡터 재구축 (스케일/쉐어링 제거의 핵심)
    // 월드 행렬의 X축(0열)을 쓰지 않고 외적으로 새로 구합니다.
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right;
    
    // 짐벌락(Gimbal Lock) 예외 처리: 카메라가 수직으로 위/아래를 볼 때
    if (glm::abs(glm::dot(forward, worldUp)) > 0.999f) {
        // 위를 보고 있으면 Right를 임의의 축(예: X축)으로 설정
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(glm::cross(forward, worldUp));
    }

    // 4. Up(위) 벡터 재구축
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // 5. View Matrix 직접 조립 (R_transposed * T_inverse)
    // GLM은 Column-Major이므로 [Col][Row] 순서 혹은 생성자에 열 순서로 대입
    glm::mat4 view(1.0f);

    // -- 회전 파트 (Transposed Rotation) --
    // Right Vector (1행)
    view[0][0] = right.x;
    view[1][0] = right.y;
    view[2][0] = right.z;

    // Up Vector (2행)
    view[0][1] = up.x;
    view[1][1] = up.y;
    view[2][1] = up.z;

    // Backward Vector (3행) - OpenGL 뷰 공간은 Z가 뒤쪽을 향함
    // forward의 반대인 backward(-forward)가 필요하지만,
    // 위에서 구한 forward 벡터를 기준으로 생각하면: -forward
    glm::vec3 viewZ = -forward; 
    view[0][2] = viewZ.x;
    view[1][2] = viewZ.y;
    view[2][2] = viewZ.z;

    // -- 이동 파트 (Translation) --
    // 공식: -dot(Axis, Position)
    view[3][0] = -glm::dot(right, pos);
    view[3][1] = -glm::dot(up, pos);
    view[3][2] = -glm::dot(viewZ, pos);
    
    // 마지막 3,3은 1.0f (초기화시 설정됨)

    return view;
}


void RenderSystemModule::RegisterSystem(flecs::world &world)
{
    renderQuery =
        world.query_builder<const MeshFilter, const MeshRenderer, const TransformMatrix>()
            .cached()
            .term_at(2)
            .second<World>()
            .build();

    world.system<const Camera, const TransformMatrix, const RenderTarget *>("RenderSystem")
        .kind(flecs::OnStore)
        .term_at(1)
        .second<World>()
        .term_at(2)
        .optional()
        .each([&](flecs::entity camEnt, const Camera &cam, const TransformMatrix &cameraWorldMatrix, const RenderTarget *target)
              {
        // --- [A] 렌더 타겟 설정 (어디에 그릴지) ---

        float aspect = 1.77f; // 안전장치
                       
        if (target && target->fbo) {
            target->fbo->Bind();
            glViewport(0, 0, target->fbo->GetWidth(), target->fbo->GetHeight());
            aspect=(float)target->fbo->GetWidth() / target->fbo->GetHeight();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, (int)m_WindowSize.width, (int)m_WindowSize.height);
            aspect=m_WindowSize.GetAspect();
        }

        if(aspect==0.0f)
            aspect=1.77f;
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 화면 비우기

        glm::mat4 viewMatrix = CreateViewMatrixFromWorld(cameraWorldMatrix);
        glm::vec3 viewPos = glm::vec3(cameraWorldMatrix*glm::vec4(0.f, 0.f, 0.f, 1.f));
        // std::cout<<"cam name: "<<camEnt.name()<<'\n';
        // std::cout<<"pos: "<<viewPos.x<<", "<<viewPos.y<<", "<<viewPos.z<<'\n';

        glm::mat4 projection = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);

        // 쉐이더 바인딩 및 '공통 유니폼' 설정 (카메라 당 1회)
        m_SimpleShader->Bind();
        m_SimpleShader->SetMat4("u_View", viewMatrix);
        m_SimpleShader->SetMat4("u_Projection", projection);
        m_SimpleShader->SetFloat4("u_Color", {0.8f, 0.8f, 0.8f, 1.f}); // 조명색 등
        m_SimpleShader->SetFloat3("u_ViewPos", viewPos);


        // --- [C] 세상의 모든 메쉬 그리기 (Nested Loop) ---
        // 여기서 아까 만들어둔 renderQuery를 돌립니다.
        // 기존 코드의 'C. 메쉬 그리기' 부분이 여기 들어옵니다.
        
        if(renderQuery)
        {
            renderQuery.each([&](flecs::entity entity, const MeshFilter& mf, const MeshRenderer& mr, const TransformMatrix& tm)
            {
                m_SimpleShader->SetMat4("u_Model", tm);
                // 해당 메쉬의 VAO 바인딩
                auto meshData = AssetManager::Get().GetMesh(mf.meshID);
                meshData->vao->Bind();
                
                // 서브메쉬 단위로 그리기
                int index=0;
                for (const auto &sub : meshData->subMeshes)
                {
                    const MaterialData* mat=AssetManager::Get().GetMaterial(mr.materials[index]);
                    m_SimpleShader->SetFloat4("u_Color", mat->baseColorFactor); // 조명색 등
                    glDrawElements(GL_TRIANGLES,
                                sub.indexCount,
                                GL_UNSIGNED_INT,
                                (void *)(sub.indexStart * sizeof(uint32_t)));
                    index++;
                }
            });
        }
        // 타겟 언바인딩 (필요시)
        if (target && target->fbo) target->fbo->Unbind(); 
    });

    // // //todo: handle frame buffers
    // world.system<const MeshFilter, const MeshRenderer, const TransformMatrix>("RenderSystem")
    // .term_at(2).second<World>()
    // .each([&](flecs::entity entity, const MeshFilter& mf, const MeshRenderer& mr, const TransformMatrix& tm)
    // {
    //     // 1. 카메라 행렬 설정
    //     glm::vec3 viewPos = glm::vec3(0.0f, 0.5f, 1.0f);
    //     glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    //     float aspect = m_WindowSize.GetAspect();
    //     if (aspect == 0.0f)
    //         aspect = 1.77f; // 안전장치
    //     glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    //     // 2. 쉐이더 공통 유니폼 설정
    //     m_SimpleShader->Bind();
    //     m_SimpleShader->SetMat4("u_View", view);
    //     m_SimpleShader->SetMat4("u_Projection", projection);
    //     m_SimpleShader->SetFloat4("u_Color", {0.8f, 0.8f, 0.8f, 1.f});
    //     m_SimpleShader->SetFloat3("u_ViewPos", viewPos);

    //     // C. 메쉬 그리기
    //     {
    //         m_SimpleShader->SetMat4("u_Model", tm);

    //         // 해당 메쉬의 VAO 바인딩
    //         auto meshData = AssetManager::Get().GetMesh(mf.meshID);
    //         meshData->vao->Bind();

    //         // 서브메쉬 단위로 그리기
    //         for (const auto &sub : meshData->subMeshes)
    //         {
    //             glDrawElements(GL_TRIANGLES,
    //                         sub.indexCount,
    //                         GL_UNSIGNED_INT,
    //                         (void *)(sub.indexStart * sizeof(uint32_t)));
    //         }
    //     }
    // });
}