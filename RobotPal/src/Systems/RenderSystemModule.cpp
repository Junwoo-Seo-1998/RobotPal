#include "RobotPal/Systems/RenderSystemModule.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Core/IBLBaker.h"
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

    world.observer<const Skybox>("OnSkyboxChange")
        .event(flecs::OnSet)
        .each([&](flecs::entity e, const Skybox &skybox)
              {
            // 1. 굽기 (기존 클래스들을 활용해 내부적으로 처리)
            GlobalLighting lighting = IBLBaker::Bake(skybox.textureID);
            
            // 2. 결과 저장
            e.world().set<GlobalLighting>(lighting); });

    InitSkybox();
    RegisterSystem(world);
}

glm::mat4 CreateViewMatrixFromWorld(const glm::mat4 &worldMatrix)
{
    // 1. 위치(Position) 추출 (4열)
    glm::vec3 pos = glm::vec3(worldMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f));

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
    if (glm::abs(glm::dot(forward, worldUp)) > 0.999f)
    {
        // 위를 보고 있으면 Right를 임의의 축(예: X축)으로 설정
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
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

        // [1] 쉐이더 준비 (미리 컴파일된 PBR 쉐이더 가져오기)
        auto pbrShader = AssetManager::Get().GetShader("./Assets/Shaders/PBR.glsl");
        pbrShader->Bind();

        // [2] 공통 유니폼 설정 (카메라 정보)
        pbrShader->SetMat4("u_View", viewMatrix);
        pbrShader->SetMat4("u_Projection", projection);
        pbrShader->SetFloat3("u_ViewPos", viewPos);

        // [3] IBL 데이터 바인딩 (GlobalLighting 컴포넌트)
        // ECS 월드에서 GlobalLighting 데이터(싱글톤)를 가져옵니다.
        auto ibl=world.try_get<GlobalLighting>();
        if (ibl) 
        {
            pbrShader->SetInt("u_UseIBL", 1);

            // 3-1. Diffuse: SH 계수 9개 전송
            for (int i = 0; i < 9; ++i) {
                std::string name = "u_SH[" + std::to_string(i) + "]";
                pbrShader->SetFloat3(name, ibl->shCoeffs[i]);
            }

            // 3-2. Specular: Prefiltered Map (Slot 5)
            // 텍스처 객체를 가져와서 바인딩 (ID만 있으면 AssetManager 통해서 가져옴)
            auto prefilterTex = AssetManager::Get().GetTextureHDR(ibl->prefilteredMap);
            if (prefilterTex) {
                prefilterTex->Bind(5); // Slot 5 활성화
                pbrShader->SetInt("u_PrefilterMap", 5);
            }

            // 3-3. Specular: BRDF LUT (Slot 6)
            auto brdfTex = AssetManager::Get().GetTextureHDR(ibl->brdfLUT);
            if (brdfTex) {
                brdfTex->Bind(6); // Slot 6 활성화
                pbrShader->SetInt("u_BRDFLUT", 6);
            }
        } 
        else 
        {
            // IBL 데이터가 없으면 끔 (Fallback Lighting 사용)
            pbrShader->SetInt("u_UseIBL", 0);
        }

        // [4] 메쉬 그리기 (PBR 재질 적용)
        if(renderQuery)
        {
            renderQuery.each([&](flecs::entity entity, const MeshFilter& mf, const MeshRenderer& mr, const TransformMatrix& tm)
            {
                pbrShader->SetMat4("u_Model", tm);

                // VAO 바인딩
                auto meshData = AssetManager::Get().GetMesh(mf.meshID);
                if (!meshData) return; // 안전장치
                meshData->vao->Bind();
                
                // 서브메쉬 순회
                int index = 0;
                for (const auto &sub : meshData->subMeshes)
                {
                    // 재질 데이터 가져오기
                    const MaterialData* mat = nullptr;
                    if (index < mr.materials.size()) 
                    {
                        mat = AssetManager::Get().GetMaterial(mr.materials[index]);
                    }

                    if (mat) 
                    {
                        // PBR 재질 속성 전송
                        // glTF 로더가 파싱한 값들 (없으면 기본값 사용)
                        pbrShader->SetFloat3("u_Albedo", glm::vec3(mat->baseColorFactor)); 
                        pbrShader->SetFloat("u_Metallic", 1.0f);
                        pbrShader->SetFloat("u_Roughness", 1.0f);
                        pbrShader->SetFloat("u_AO", 1.0f); // AO 맵이 없다면 기본 1.0
                    } 
                    else 
                    {
                        // 재질이 없는 경우 기본값 (핑크색 등)
                        pbrShader->SetFloat3("u_Albedo", {1.0f, 0.0f, 1.0f});
                        pbrShader->SetFloat("u_Metallic", 0.0f);
                        pbrShader->SetFloat("u_Roughness", 0.5f);
                        pbrShader->SetFloat("u_AO", 1.0f);
                    }

                    glDrawElements(GL_TRIANGLES,
                                sub.indexCount,
                                GL_UNSIGNED_INT,
                                (void *)(sub.indexStart * sizeof(uint32_t)));
                    index++;
                }
            });
        }

        auto env=world.try_get<GlobalLighting>();
        if (env && env->environmentMap) 
        {   // IBL 데이터가 있을 때만
            // 중요: 깊이 함수를 LEQUAL로 변경 
            // (Skybox는 z=1.0인데 초기화 값도 1.0이므로 '같거나 작음'이어야 그려짐)
            glDepthFunc(GL_LEQUAL);
                 
            m_SkyboxShader->Bind();
            m_SkyboxShader->SetMat4("u_View", viewMatrix);
            m_SkyboxShader->SetMat4("u_Projection", projection);
            m_SkyboxShader->SetFloat("u_Intensity", env->intensity);

            // 텍스처 바인딩
            auto envTex = AssetManager::Get().GetTextureHDR(env->environmentMap);
            if(envTex) 
            {
                envTex->Bind(0);
                m_SkyboxShader->SetInt("u_EnvironmentMap", 0);
                     
                // 큐브 그리기
                m_SkyboxVAO->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                m_SkyboxVAO->UnBind();
            }

            // 깊이 함수 원복 (다음 프레임을 위해)
            glDepthFunc(GL_LESS);
        }
        // 타겟 언바인딩 (필요시)
        if (target && target->fbo) target->fbo->Unbind(); 
    });
}
void RenderSystemModule::InitSkybox()
{
    m_SkyboxShader = AssetManager::Get().GetShader("Assets/Shaders/Skybox.glsl");

    // 2. 큐브 VAO 생성 (IBLBaker와 별도로 관리하는게 안전함)
    float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};
    m_SkyboxVAO = VertexArray::Create();
    auto vb = std::make_shared<VertexBuffer>(skyboxVertices, sizeof(skyboxVertices));
    vb->SetLayout({{DataType::Float3, "a_Position"}});
    m_SkyboxVAO->AddVertexBuffer(vb);
}