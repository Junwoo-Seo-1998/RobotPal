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
#include "RobotPal/Streaming/IStreamingManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include "imgui_internal.h"
#include <glad/gles2.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>


static std::unique_ptr<NetworkEngine> g_NetworkEngine;
static std::unique_ptr<IRobotController> g_Controller;
static Entity g_RobotEntity;

std::shared_ptr<Framebuffer> camView;
void SandboxScene::OnEnter()
{
    m_StreamingManager = IStreamingManager::Create();
    if (m_StreamingManager)
    {
        m_StreamingManager->Init();
    }

    auto hdrID = AssetManager::Get().LoadTextureHDR("./Assets/airport.hdr");
    m_World.set<Skybox>({hdrID, 1.0f, 0.0f});

    auto modelPrefab = AssetManager::Get().GetPrefab(m_World, "/Assets/jetank.glb");
    auto prefabEntity = CreateEntity("mainModel");
    prefabEntity.GetHandle().is_a(modelPrefab);

    prefabEntity.SetLocalPosition(glm::vec3(0.f, 0.f, 0.7f));
    prefabEntity.SetLocalRotation(glm::radians(glm::vec3(0.f, -90.f, 0.f)));

    auto prefabEntity2 = CreateEntity("mainModel2");
    prefabEntity2.GetHandle().is_a(modelPrefab);
    prefabEntity2.SetLocalPosition({0.4f, 0.f, 0.5f});
    prefabEntity2.SetLocalRotation(glm::radians(glm::vec3(0.f, -211.f, 0.f)));

    auto mapPrefab = AssetManager::Get().GetPrefab(m_World, "/Assets/map.glb");
    auto map=CreateEntity("map");
    map.GetHandle().is_a(mapPrefab);

    auto mainCam=CreateEntity("mainCam");
    mainCam.Set<Camera>({});
    mainCam.SetLocalPosition({0.1f, 0.5f, 1.1f});
    mainCam.SetLocalRotation(glm::radians(glm::vec3(-35.f, -0.15f, 0.f)));

    camView=Framebuffer::Create(1640, 1232);
    auto robotCamera=CreateEntity("robotCam");
    robotCamera.Set<Camera>({160.f, 0.001f, 1000.f, true})
               .Set<RenderTarget>({camView})
               .Set<VideoSender>({400, 400, 15.0f});

    
    
    auto attachPoint=prefabEntity.FindChildByNameRecursive(prefabEntity, "Cam");
    if(attachPoint)
    {
        robotCamera.SetParent(attachPoint);
        robotCamera.SetLocalPosition({0.f,0.f,-0.001f});
    }
    
}  

void SandboxScene::OnUpdate(float dt)
{
    // --- Video Streaming Logic ---
    m_timeSinceLastSend += dt;

    // Find the entity with a VideoSender component
    auto videoSenderQuery = m_World.query<VideoSender, const RenderTarget>();

    videoSenderQuery.each([&](flecs::entity e, VideoSender& sender, const RenderTarget& target) {
        
        float interval = 1.0f / sender.fpsLimit;
        if (m_timeSinceLastSend < interval) {
            return; // Not time to send yet
        }
        
        m_timeSinceLastSend = 0.f; // Reset timer

        // Check for valid streaming manager and connection
        if (!m_StreamingManager || !m_StreamingManager->IsConnected()) {
            return;
        }

        // Get the framebuffer and its color attachment (the texture to be sent)
        auto fbo = target.fbo;
        if (!fbo || !fbo->GetColorAttachment()) {
            return;
        }

        // Read pixel data from the GPU asynchronously.
        // This function returns data from the *previous* frame, which is fine for streaming.
        std::vector<uint8_t> pixels = fbo->GetColorAttachment()->GetAsyncData();
        if (pixels.empty()) {
            return; // PBO is not ready on the first frame, will be available next time.
        }
        
        // Populate the frame data structure
        FrameData frame;
        frame.width = fbo->GetWidth();
        frame.height = fbo->GetHeight();
        
        // We know the framebuffer was created with RGB8 format (3 channels)
        // frame.channels = 4;
        frame.channels = 4; 
        frame.pixel_data = pixels;

        // Send the frame over the network
        m_StreamingManager->SendFrame(frame);
    });
}

void SandboxScene::OnExit()
{
    if (m_StreamingManager)
    {
        m_StreamingManager->Shutdown();
    }
}
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

    // --- 1. Streaming Manager UI (왼쪽 위) ---
    ImGui::Begin("Streaming Controls");
    if (m_StreamingManager)
    {
        ImGui::Text("Status: %s", m_StreamingManager->GetStatusMessage().c_str());
        if (m_StreamingManager->IsConnected())
        {
            if (ImGui::Button("Disconnect"))
                m_StreamingManager->Disconnect();
        }
        else
        {
            if (ImGui::Button("Connect"))
                m_StreamingManager->ConnectToServer("ws://127.0.0.1:9999");
        }
    }
    else
    {
        ImGui::Text("Mgr Not Init");
    }
    ImGui::End();
    
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
}