#include "RobotPal/SandboxScene.h"
#include "RobotPal/Buffer.h"
#include "RobotPal/SimController.h" 
#include "RobotPal/RobotController.h"
#include "RobotPal/RealController.h"
#include "RobotPal/HybridController.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/SceneManager.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Network/NetworkEngine.h"
#include "RobotPal/Util/FileDialog.h"
#include "RobotPal/Util/SceneSerializer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <glad/gles2.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <json.hpp>

static std::unique_ptr<SimController> g_Controller;
static Entity prefabEntity;
static constexpr int cam_W = 1232;
static constexpr int cam_H = 832;
std::shared_ptr<Framebuffer> camView;
void SandboxScene::OnEnter()
{    
    auto modelPrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/jetank.glb");
    prefabEntity = CreateEntity("mainModel");
    prefabEntity.GetHandle().is_a(modelPrefab);
    prefabEntity.SetLocalPosition(glm::vec3(0.f, 0.f, 0.35f));
    prefabEntity.SetLocalRotation(glm::radians(glm::vec3(0.f, -90.f, 0.f)));

    Entity ee = prefabEntity.FindChildByNameRecursive(prefabEntity.GetHandle(), "EE");
    if (ee.IsValid()) {
        ee.Set<GripperLogic>({false, 0.025f,false, Entity()}); // 범위 1.5f
    // 위치 확인용 디버깅
        std::cout << "[Init] GripperLogic added to EE.\n";
    }   
    
    // box
    
    auto boxModel = AssetManager::Get().GetPrefab(m_World, "./Assets/box.glb");
    auto boxEntity=CreateEntity("box");
    boxEntity.GetHandle().is_a(boxModel);
    boxEntity.Set<Grabbable>({});
    boxEntity.SetLocalPosition(glm::vec3(0.0f, 0.125f, 0.0f));
    boxEntity.SetLocalRotation(glm::radians(glm::vec3(0.f, 0.f, 0.f)));
    
    std::cout << "[Init] Grabbable component added to box entity.\n";
    // car
    auto modelPrefab2 = AssetManager::Get().GetPrefab(m_World, "./Assets/cars.glb");
    auto prefabEntity2 = CreateEntity("CarGroups");
    prefabEntity2.GetHandle().is_a(modelPrefab2);

    auto mapPrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/map.glb");
    auto map=CreateEntity("map");
    map.GetHandle().is_a(mapPrefab);

    // auto tilePrefab = AssetManager::Get().GetPrefab(m_World, "./Assets/tile.glb");
    // auto tile=CreateEntity("tile");
    // tile.GetHandle().is_a(tilePrefab);

    auto mainCam=CreateEntity("MainCamera");
    mainCam.Set<Camera>({});
    mainCam.SetLocalPosition({0.0f, 0.5f, 1.1f});
    mainCam.SetLocalRotation(glm::radians(glm::vec3(-35.f, -0.15f, 0.f)));

    camView=Framebuffer::Create(cam_W, cam_H);
    // camView=Framebuffer::Create(224, 224);

    auto robotCamera=CreateEntity("RobotCamera");
#ifdef __EMSCRIPTEN__
    // Web 환경: WebSocket
    robotCamera.Set<Camera>({160.f, 0.01f, 1000.f, true})
           .Set<RenderTarget>({camView})
           .Set<VideoSender>({"ws://127.0.0.1:9999"}); // WebSocket URL
#else
    // 네이티브 환경: TCP
    robotCamera.Set<Camera>({160.f, 0.01f, 1000.f, true})
           .Set<RenderTarget>({camView})
           .Set<VideoSender>({"127.0.0.1:9998"}); // TCP 소켓 주소
#endif
    
    auto attachPoint=prefabEntity.FindChildByNameRecursive(prefabEntity, "Cam");
    if(attachPoint)
    {
        robotCamera.SetParent(attachPoint);
    }

    g_Controller = std::make_unique<SimController>(prefabEntity,m_World);
    
    if (g_Controller->Init()) {
        std::cout << ">>> Hybrid Controller (Shared Entity) Initialized!" << std::endl;
    }
    std::cout << ">>> Setting ControllerComponent on prefabEntity\n";
    prefabEntity.Set<ControllerComponent>({prefabEntity});
    std::cout << ">>> ControllerComponent set successfully\n";
    flecs::world w;
    
    
    //save("test.json");
    //load("test.json");
}  

void SandboxScene::OnUpdate(float dt)
{
    if (!g_Controller) return;

    float v = 0.0f;
    float w = 0.0f;
    GLFWwindow* window = glfwGetCurrentContext();
    float speed = 0.2f; 
    float turn_speed = 1.25f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) v = speed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) v = -speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) w = turn_speed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) w = -turn_speed;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) g_Controller->TryGrip();
    g_Controller->Move(v, w);
    g_Controller->Update(dt);


}

void SandboxScene::OnExit()
{
}
void SandboxScene::OnImGuiRender()
{
    // ImGui::Begin("baked IBL");
    // //ImGui::Image((void*)(intptr_t)AssetManager::Get().GetTextureHDR(GetID("./Assets/airport.hdr"))->GetID(), ImVec2(200, 100), ImVec2(0, 0), ImVec2(1, -1));
    // ImGui::Image((void*)(intptr_t)AssetManager::Get().GetTextureHDR(GetID("Generated/IBL_Environment"))->GetID(), ImVec2(100, 100), ImVec2(0, 0), ImVec2(1, -1));
    // ImGui::Image((void*)(intptr_t)AssetManager::Get().GetTextureHDR(GetID("IBL_BRDF_LUT"))->GetID(), ImVec2(100, 100), ImVec2(0, 0), ImVec2(1, -1));
    // ImGui::End();
    // ImGui::Begin("robotCam");
    // ImGui::Image((void*)(intptr_t)camView->GetColorAttachment()->GetID(), ImVec2(cam_W, cam_H), ImVec2(0, 0), ImVec2(1, -1));
    // ImGui::End();
    
}
