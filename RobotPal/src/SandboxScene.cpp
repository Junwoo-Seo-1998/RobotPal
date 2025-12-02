#include "RobotPal/SandboxScene.h"
#include "RobotPal/Buffer.h"
#include "RobotPal/SimController.h" // SimDriver 헤더
#include "RobotPal/RobotController.h"
#include "RobotPal/RealController.h"
#include "RobotPal/HybridController.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Network/NetworkEngine.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <glad/gles2.h>
#include <GLFW/glfw3.h>
#include <iostream>

static std::unique_ptr<NetworkEngine> g_NetworkEngine;
static std::unique_ptr<IRobotController> g_Controller;
static Entity g_RobotEntity;
#include <memory>
std::shared_ptr<Framebuffer> camView;
void SandboxScene::OnEnter()
{   
    g_NetworkEngine = std::make_unique<NetworkEngine>(m_World);
    g_NetworkEngine->Connect("127.0.0.1:12345"); // 접속 시도

    auto modelPrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/jetank.glb");
    g_RobotEntity = CreateEntity("mainModel");
    g_RobotEntity.GetHandle().is_a(modelPrefab);

    g_RobotEntity.SetLocalRotation(glm::radians(glm::vec3(0.f, -135.f, 0.f)));

    auto prefabEntity2 = CreateEntity("mainModel2");
    prefabEntity2.GetHandle().is_a(modelPrefab);
    prefabEntity2.SetLocalPosition({0.2f, 0.f, 0.2f});


    auto mainCam=CreateEntity("mainCam");
    mainCam.Set<Camera>({});
    mainCam.SetLocalPosition({0.0f, 0.5f, 1.0f});

    camView=Framebuffer::Create(400, 400);
    auto robotCamera=CreateEntity("robotCam");
    robotCamera.Set<Camera>({80.f, 0.001f, 1000.f})
               .Set<RenderTarget>({camView})
               .Set<VideoSender>({400, 400, 15.0f});
    
    auto attachPoint=g_RobotEntity.FindChildByNameRecursive(g_RobotEntity, "Cam");
    if(attachPoint)
    {
        robotCamera.SetParent(attachPoint);
    }

    g_Controller = std::make_unique<HybridController>(g_RobotEntity, 12345);

    if (g_Controller->Init()) {
        std::cout << ">>> Hybrid Controller (Shared Entity) Initialized!" << std::endl;
    }

}  

void SandboxScene::OnUpdate(float dt)
{
    if (!g_Controller) return;

    // -------------------------------------------------------
    // [1] 입력 처리 (Input)
    // -------------------------------------------------------
    float v = 0.0f;
    float w = 0.0f;
    GLFWwindow* window = glfwGetCurrentContext();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) v = 2.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) v = -2.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) w = 2.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) w = -2.0f;

    // -------------------------------------------------------
    // [2] 드라이버 업데이트 (Logic)
    // -------------------------------------------------------
    // 드라이버가 Entity의 Position, Rotation을 수정함

    g_Controller->Move(v, w);
    g_Controller->Update(dt);

   

    // 모델 그리기
    // auto modelRes = AssetManager::Get().GetModel("./Assets/jetank.glb");
    // if (modelRes) {
    //     RenderNode(modelRes->nodes[modelRes->rootNodeIndex], rootTransform, *modelRes);
    // }
    //auto data=camView->GetColorAttachment()->GetAsyncData();

    //일단 이형태로 가져와서 씀
    //auto NetworkEngine=m_World.get<NetworkEngineHandle>().instance;
    
}

void SandboxScene::OnExit()
{

}
void SandboxScene::OnImGuiRender()
{
    ImGui::Begin("robotCam");
    ImGui::Image((void*)(intptr_t)camView->GetColorAttachment()->GetID(), ImVec2(400, 400), ImVec2(0, 0), ImVec2(1, -1));
    ImGui::End();

    // 창 이름을 전체를 아우르는 이름으로 변경하면 좋습니다.
    ImGui::Begin("Scene Graph & Inspector");

    // 1. 선택된 엔티티 상태
    static flecs::entity selectedEntity;

    // ---------------------------------------------------------
    // [레이아웃 시작] 2개의 컬럼으로 화면 분할 (크기 조절 가능)
    // ---------------------------------------------------------
    // flags: Resizable(경계선 드래그), BordersInnerV(중간 세로줄)
    if (ImGui::BeginTable("SplitLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        // =====================================================
        // [왼쪽 컬럼] 계층 구조 (Hierarchy)
        // =====================================================
        ImGui::TableNextColumn();
        
        // 팁: 왼쪽 영역 안에서만 스크롤이 되도록 Child Window를 사용합니다.
        // 높이를 0으로 설정하면 남은 세로 공간을 모두 채웁니다.
        ImGui::BeginChild("HierarchyRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        ImGui::TextDisabled("Hierarchy");
        ImGui::Separator();

        // --- 재귀 람다 (기존 로직 유지) ---
        auto drawNode = [&](auto&& self, flecs::entity e) -> void {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (selectedEntity == e) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool hasChildren = false;
            e.children([&](flecs::entity) { hasChildren = true; });
            if (!hasChildren) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            if (e.name() == "mainModel") {
                flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }

            const char* name = e.name().c_str();
            std::string label = (name && *name) ? name : std::to_string(e.id());

            bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)e.id(), flags, "%s", label.c_str());

            if (ImGui::IsItemClicked()) {
                selectedEntity = e;
            }

            if (opened) {
                if (hasChildren) {
                    e.children([&](flecs::entity child) {
                        self(self, child);
                    });
                }
                ImGui::TreePop();
            }
        };

        // --- 트리 그리기 ---
        flecs::entity rootModel = m_SceneRoot;
        if (rootModel.is_alive()) {
            rootModel.children([&](flecs::entity child) {
                drawNode(drawNode, child);
            });
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "'mainModel' not found.");
        }

        ImGui::EndChild(); // HierarchyRegion 끝

        // =====================================================
        // [오른쪽 컬럼] 인스펙터 (Inspector)
        // =====================================================
        ImGui::TableNextColumn();
        
        // 오른쪽 영역도 내용이 많으면 스크롤 되도록 Child Window 처리
        ImGui::BeginChild("InspectorRegion", ImVec2(0, 0), false);

        ImGui::TextDisabled("Inspector");
        ImGui::Separator();

        if (selectedEntity.is_alive()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%s] Properties", selectedEntity.name().c_str());
            ImGui::Spacing();

            // 사용자 정의 Entity 래퍼 사용
            Entity selectedEntityWrapper(selectedEntity);

            // 1. Position (Translation)
            {
                auto pos = selectedEntityWrapper.GetLocalPosition();
                if(ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f))
                {
                    selectedEntityWrapper.SetLocalPosition(pos);
                }
            }

            // 2. Rotation (Radian <-> Degree 변환)
            {
                // GetLocalRotation()이 Radian을 반환한다고 가정
                glm::vec3 eulerAngles = glm::degrees(selectedEntityWrapper.GetLocalRotation());
                
                // UI에서는 Degree로 조작
                if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) 
                {
                    // 저장할 때는 다시 Radian으로
                    selectedEntityWrapper.SetLocalRotation(glm::radians(eulerAngles));
                }
            }

            // 3. Scale
            {
                auto scale = selectedEntityWrapper.GetLocalScale();
                if(ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.05f))
                {
                    selectedEntityWrapper.SetLocalScale(scale);
                }
            }
        } else {
            ImGui::TextDisabled("No entity selected.");
        }
        
        ImGui::EndChild(); // InspectorRegion 끝
        
        ImGui::EndTable(); // SplitLayout 끝
    }

    ImGui::End();
}