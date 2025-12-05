#include "RobotPal/SandboxScene.h"
#include "RobotPal/Network/NetworkTransport.h"
#include "RobotPal/Network/WebSocketTransport.h" // WebSocket 구현
#include "RobotPal/Network/TcpNetworkTransport.h" // WebSocket 구현
#include "RobotPal/Core/Texture.h"             // GetAsyncData 사용 위해
#include "RobotPal/Core/Framebuffer.h"         // Framebuffer 사용 위해
#include "RobotPal/Buffer.h"
#include "RobotPal/SimController.h" 
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
#include "imgui_internal.h"
#include <iostream>
#include <memory>



static std::unique_ptr<IRobotController> g_Controller;
static Entity g_RobotEntity;

std::shared_ptr<Framebuffer> camView;
void SandboxScene::OnEnter()
{   
    auto hdrID = AssetManager::Get().LoadTextureHDR("./Assets/airport.hdr");
    m_World.set<Skybox>({hdrID, 1.0f, 0.0f});

    auto modelPrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/jetank.glb");
    auto prefabEntity = CreateEntity("mainModel");
    prefabEntity.GetHandle().is_a(modelPrefab);

    prefabEntity.SetLocalPosition(glm::vec3(0.f, 0.f, 0.7f));
    prefabEntity.SetLocalRotation(glm::radians(glm::vec3(0.f, -90.f, 0.f)));

    auto prefabEntity2 = CreateEntity("mainModel2");
    prefabEntity2.GetHandle().is_a(modelPrefab);
    prefabEntity2.SetLocalPosition({0.4f, 0.f, 0.5f});
    prefabEntity2.SetLocalRotation(glm::radians(glm::vec3(0.f, -211.f, 0.f)));

    auto mapPrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/map.glb");
    auto map=CreateEntity("map");
    map.GetHandle().is_a(mapPrefab);

    auto mainCam=CreateEntity("mainCam");
    mainCam.Set<Camera>({});
    mainCam.SetLocalPosition({0.1f, 0.5f, 1.1f});
    mainCam.SetLocalRotation(glm::radians(glm::vec3(-35.f, -0.15f, 0.f)));

    camView=Framebuffer::Create(400, 400);
    auto robotCamera=CreateEntity("robotCam");
    robotCamera.Set<Camera>({80.f, 0.001f, 1000.f})
               .Set<RenderTarget>({camView})
               .Set<VideoSender>({"127.0.0.1:9998"});
    
    auto attachPoint=prefabEntity.FindChildByNameRecursive(prefabEntity, "Cam");
    if(attachPoint)
    {
        robotCamera.SetParent(attachPoint);
    }

    g_Controller = std::make_unique<HybridController>(prefabEntity);

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
    float speed = 1.0f; 
    float turn_speed = 2.5f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) v = speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) v = -speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) w = turn_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) w = -turn_speed;

    // -------------------------------------------------------
    // [2] 드라이버 업데이트 (Logic)
    // -------------------------------------------------------
    // 드라이버가 Entity의 Position, Rotation을 수정함

    g_Controller->Move(v, w);
    g_Controller->Update(dt);
}

void SandboxScene::OnExit()
{
}
// void SandboxScene::OnImGuiRender()
// {
//     // ImGui::Begin("baked IBL");
//     // //ImGui::Image((void*)(intptr_t)AssetManager::Get().GetTextureHDR(GetID("./Assets/airport.hdr"))->GetID(), ImVec2(200, 100), ImVec2(0, 0), ImVec2(1, -1));
//     // ImGui::Image((void*)(intptr_t)AssetManager::Get().GetTextureHDR(GetID("Generated/IBL_Environment"))->GetID(), ImVec2(100, 100), ImVec2(0, 0), ImVec2(1, -1));
//     // ImGui::Image((void*)(intptr_t)AssetManager::Get().GetTextureHDR(GetID("IBL_BRDF_LUT"))->GetID(), ImVec2(100, 100), ImVec2(0, 0), ImVec2(1, -1));
//     // ImGui::End();

//     ImGui::Begin("robotCam");
//     ImGui::Image((void*)(intptr_t)camView->GetColorAttachment()->GetID(), ImVec2(400, 400), ImVec2(0, 0), ImVec2(1, -1));
//     ImGui::End();

//     // 창 이름을 전체를 아우르는 이름으로 변경하면 좋습니다.
//     ImGui::Begin("Scene Graph & Inspector");

//     // 1. 선택된 엔티티 상태
//     static flecs::entity selectedEntity;

//     // ---------------------------------------------------------
//     // [레이아웃 시작] 2개의 컬럼으로 화면 분할 (크기 조절 가능)
//     // ---------------------------------------------------------
//     // flags: Resizable(경계선 드래그), BordersInnerV(중간 세로줄)
//     if (ImGui::BeginTable("SplitLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
//     {
//         // =====================================================
//         // [왼쪽 컬럼] 계층 구조 (Hierarchy)
//         // =====================================================
//         ImGui::TableNextColumn();
        
//         // 팁: 왼쪽 영역 안에서만 스크롤이 되도록 Child Window를 사용합니다.
//         // 높이를 0으로 설정하면 남은 세로 공간을 모두 채웁니다.
//         ImGui::BeginChild("HierarchyRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
//         ImGui::TextDisabled("Hierarchy");
//         ImGui::Separator();

//         // --- 재귀 람다 (기존 로직 유지) ---
//         auto drawNode = [&](auto&& self, flecs::entity e) -> void {
//             ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
//             if (selectedEntity == e) {
//                 flags |= ImGuiTreeNodeFlags_Selected;
//             }

//             bool hasChildren = false;
//             e.children([&](flecs::entity) { hasChildren = true; });
//             if (!hasChildren) {
//                 flags |= ImGuiTreeNodeFlags_Leaf;
//             }

//             if (e.name() == "mainModel") {
//                 flags |= ImGuiTreeNodeFlags_DefaultOpen;
//             }

//             const char* name = e.name().c_str();
//             std::string label = (name && *name) ? name : std::to_string(e.id());

//             bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)e.id(), flags, "%s", label.c_str());

//             if (ImGui::IsItemClicked()) {
//                 selectedEntity = e;
//             }

//             if (opened) {
//                 if (hasChildren) {
//                     e.children([&](flecs::entity child) {
//                         self(self, child);
//                     });
//                 }
//                 ImGui::TreePop();
//             }
//         };

//         // --- 트리 그리기 ---
//         flecs::entity rootModel = m_SceneRoot;
//         if (rootModel.is_alive()) {
//             rootModel.children([&](flecs::entity child) {
//                 drawNode(drawNode, child);
//             });
//         } else {
//             ImGui::TextColored(ImVec4(1, 0, 0, 1), "'mainModel' not found.");
//         }

//         ImGui::EndChild(); // HierarchyRegion 끝

//         // =====================================================
//         // [오른쪽 컬럼] 인스펙터 (Inspector)
//         // =====================================================
//         ImGui::TableNextColumn();
        
//         // 오른쪽 영역도 내용이 많으면 스크롤 되도록 Child Window 처리
//         ImGui::BeginChild("InspectorRegion", ImVec2(0, 0), false);

//         ImGui::TextDisabled("Inspector");
//         ImGui::Separator();

//         if (selectedEntity.is_alive()) {
//             ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%s] Properties", selectedEntity.name().c_str());
//             ImGui::Spacing();

//             // 사용자 정의 Entity 래퍼 사용
//             Entity selectedEntityWrapper(selectedEntity);

//             // 1. Position (Translation)
//             {
//                 auto pos = selectedEntityWrapper.GetLocalPosition();
//                 if(ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f))
//                 {
//                     selectedEntityWrapper.SetLocalPosition(pos);
//                 }
//             }

//             // 2. Rotation (Radian <-> Degree 변환)
//             {
//                 // GetLocalRotation()이 Radian을 반환한다고 가정
//                 glm::vec3 eulerAngles = glm::degrees(selectedEntityWrapper.GetLocalRotation());
                
//                 // UI에서는 Degree로 조작
//                 if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) 
//                 {
//                     // 저장할 때는 다시 Radian으로
//                     selectedEntityWrapper.SetLocalRotation(glm::radians(eulerAngles));
//                 }
//             }

//             // 3. Scale
//             {
//                 auto scale = selectedEntityWrapper.GetLocalScale();
//                 if(ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.05f))
//                 {
//                     selectedEntityWrapper.SetLocalScale(scale);
//                 }
//             }
//         } else {
//             ImGui::TextDisabled("No entity selected.");
//         }
        
//         ImGui::EndChild(); // InspectorRegion 끝
        
//         ImGui::EndTable(); // SplitLayout 끝
//     }

//     ImGui::End();
// }
void SandboxScene::OnImGuiRender()
{
    // 1. 메인 컨테이너 (대시보드) 설정
    // 이 윈도우가 3개의 창을 담을 '부모' 역할을 합니다.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // 최초 실행 시 위치: (0, 0) - 왼쪽 위
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_FirstUseEver);
    // 최초 실행 시 크기: 화면 너비의 50%, 높이의 50% (전체 화면의 1/4)
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x * 0.5f, viewport->WorkSize.y * 0.5f), ImGuiCond_FirstUseEver);

    // 메인 컨테이너 윈도우 시작
    // NoDocking 플래그를 빼서 이 윈도우 자체가 다른 곳에 붙는 것을 방지할 수도 있지만, 
    // 여기서는 내부 DockSpace를 위해 기본 윈도우를 사용합니다.
    ImGui::Begin("Dashboard Control", nullptr, ImGuiWindowFlags_NoCollapse); 

    // 2. 컨테이너 내부에 DockSpace 생성
    ImGuiID dockspace_id = ImGui::GetID("MyDashboardDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // 3. 최초 실행 시 레이아웃 자동 설정
    static bool first_time = true;
    if (first_time)
    {
        first_time = false;

        ImGui::DockBuilderRemoveNode(dockspace_id); // 기존 정보 초기화
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(viewport->WorkSize.x * 0.5f, viewport->WorkSize.y * 0.5f));

        // --- 레이아웃 분할 (비율 조절) ---
        // 전체를 좌(Left) / 우(Right)로 분할. 오른쪽이 로봇 카메라(메인)이므로 60% 할당
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.6f, nullptr, &dockspace_id);
        
        // 남은 왼쪽(dockspace_id)을 다시 위(Up) / 아래(Down)로 분할
        // 위쪽: 소켓 연결 창 (작아도 됨, 30%)
        // 아래쪽: 인스펙터 (나머지)
        ImGuiID dock_id_left_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.3f, nullptr, &dockspace_id);
        
        // --- 윈도우 도킹 ---
        // 생성될 윈도우 이름과 노드 ID를 매핑
        ImGui::DockBuilderDockWindow("robotCam", dock_id_right);             // 오른쪽
        ImGui::DockBuilderDockWindow("Streaming Controls", dock_id_left_top); // 왼쪽 위
        ImGui::DockBuilderDockWindow("Scene Graph & Inspector", dockspace_id); // 왼쪽 아래 (나머지)

        ImGui::DockBuilderFinish(dockspace_id);
    }
    ImGui::End(); // Dashboard Control 윈도우 끝


    // =========================================================
    // 아래 3개의 창은 위에서 정의한 "Dashboard Control" 내부의 DockSpace에 들어갑니다.
    // (코드는 이전과 동일하며 위치만 자동으로 잡힙니다)
    // =========================================================

    // --- 1. Streaming Controls (Network Transport UI) ---
    
    // --- 2. Robot Camera View (오른쪽) ---
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("robotCam");
    
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (camView && camView->GetColorAttachment()) {
        ImGui::Image((void*)(intptr_t)camView->GetColorAttachment()->GetID(), viewportSize, ImVec2(0, 1), ImVec2(1, 0));
    } else {
        ImGui::Text("No Signal");
    }
    
    ImGui::End();
    ImGui::PopStyleVar();

    // --- 3. Scene Graph & Inspector (왼쪽 아래) ---
    ImGui::Begin("Scene Graph & Inspector");
    
    // (기존 인스펙터 코드 유지)
    static flecs::entity selectedEntity;
    if (ImGui::BeginTable("SplitLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableNextColumn();
        ImGui::BeginChild("HierarchyRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        ImGui::TextDisabled("Hierarchy");
        ImGui::Separator();
        // ... 트리 노드 그리는 코드 (생략 없이 사용하시면 됩니다) ...
        auto drawNode = [&](auto&& self, flecs::entity e) -> void {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (selectedEntity == e) flags |= ImGuiTreeNodeFlags_Selected;
            bool hasChildren = false;
            e.children([&](flecs::entity) { hasChildren = true; });
            if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
            if (e.name() == "mainModel") flags |= ImGuiTreeNodeFlags_DefaultOpen;

            const char* name = e.name().c_str();
            std::string label = (name && *name) ? name : std::to_string(e.id());

            bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)e.id(), flags, "%s", label.c_str());
            if (ImGui::IsItemClicked()) selectedEntity = e;
            if (opened) {
                if (hasChildren) {
                    e.children([&](flecs::entity child) { self(self, child); });
                }
                ImGui::TreePop();
            }
        };
        flecs::entity rootModel = m_SceneRoot;
        if (rootModel.is_alive()) {
            rootModel.children([&](flecs::entity child) { drawNode(drawNode, child); });
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("InspectorRegion", ImVec2(0, 0), false);
        ImGui::TextDisabled("Inspector");
        ImGui::Separator();

        if (selectedEntity.is_alive()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%s]", selectedEntity.name().c_str());
            Entity selectedEntityWrapper(selectedEntity);

            auto pos = selectedEntityWrapper.GetLocalPosition();
            if(ImGui::DragFloat3("Pos", glm::value_ptr(pos), 0.1f)) selectedEntityWrapper.SetLocalPosition(pos);

            glm::vec3 euler = glm::degrees(selectedEntityWrapper.GetLocalRotation());
            if (ImGui::DragFloat3("Rot", glm::value_ptr(euler), 1.0f)) selectedEntityWrapper.SetLocalRotation(glm::radians(euler));

            auto scale = selectedEntityWrapper.GetLocalScale();
            if(ImGui::DragFloat3("Scl", glm::value_ptr(scale), 0.05f)) selectedEntityWrapper.SetLocalScale(scale);
        } else {
            ImGui::TextDisabled("No Selection");
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
    ImGui::End();

    // --- 스트리밍 디버그 UI ---
    ImGui::Begin("Streaming Debug");

    // flecs 월드에서 NetworkEngineHandle을 가져옵니다.
    auto networkEngineHandle = m_World.get<const NetworkEngineHandle>();

    if (networkEngineHandle.instance)
    {
        auto netEngine = networkEngineHandle.instance;

        // 수동 연결 버튼
        if (ImGui::Button("Manual Connect to 127.0.0.1:9998"))
        {
            std::cout << "[SandboxScene] Manual connect button clicked.\n";
            netEngine->TryConnect("127.0.0.1:9998");
        }

        // 연결 상태 표시
        bool connected = netEngine->IsConnected();
        ImGui::Text("Is Connected: %s", connected ? "True" : "False");
    }
    else
    {
        ImGui::Text("Error: NetworkEngineHandle not found in world!");
    }
    ImGui::End();
}