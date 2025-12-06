#include "RobotPal/EngineApp.h"
#include "RobotPal/Window.h"
#include "RobotPal/ImGuiManager.h"
#include "RobotPal/Util/emscripten_mainloop.h"
#include "RobotPal/SceneManager.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include "RobotPal/Core/RenderCommand.h"
#include "RobotPal/VertexArray.h"
#include "RobotPal/Buffer.h"
#include "RobotPal/Shader.h"

#include "RobotPal/Network/NetworkEngine.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/SandboxScene.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Systems/RenderSystemModule.h"
#include "RobotPal/Systems/TransformSystemModule.h"
#include "RobotPal/Systems/StreamingSystemModule.h"
#include "RobotPal/Core/Texture.h"

#include <thread>
#include <chrono>

void EngineApp::Run()
{
    Init();
    MainLoop();
    Shutdown();
}

void EngineApp::Init()
{
    m_Window=std::make_shared<Window>(1280, 720, "RobotPal");
    m_Window->Init();
    ImGuiManager::Get().Init(m_Window->GetNativeWindow());

    m_SceneManager = std::make_shared<SceneManager>(m_World);
    m_SceneManager->LoadScene<SandboxScene>();

    m_World.set<WindowData>({ (float)1280, (float)720});

    m_World.import<NetworkEngine>();
    m_World.import<RenderSystemModule>();
    m_World.import<TransformSystemModule>();
    m_World.import<StreamingSystemModule>();
    
}

void EngineApp::MainLoop()
{
    m_LastFrameTime=(float)glfwGetTime();
    glm::vec4 clear_color = {0.45f, 0.55f, 0.60f, 1.00f};

    RenderCommand::Init(); 
#ifdef __EMSCRIPTEN__
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!m_Window->ShouldClose())
#endif
    {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - m_LastFrameTime;
        m_LastFrameTime = currentFrame;

        if (dt > 0.1f) dt = 0.1f;

        m_Window->PollEvents();
        if (m_Window->IsMinimized())
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        
        // Clear the screen
        int display_w, display_h;
        glfwGetFramebufferSize((GLFWwindow*)m_Window->GetNativeWindow(), &display_w, &display_h);
        m_World.set<WindowData>({ (float)display_w, (float)display_h});

        RenderCommand::SetViewport(0,0, display_w, display_h);
        RenderCommand::SetClearColor(clear_color);
        RenderCommand::Clear();
        
        m_SceneManager->OnUpdate(dt);
        m_World.progress(dt);

        // Start the Dear ImGui frame
        ImGuiManager::Get().NewFrame();

        //draw gui
        {
            m_SceneManager->OnImGuiRender();
        }

        ImGuiManager::Get().PrepareRender();
        ImGuiManager::Get().Render(m_Window->GetNativeWindow());
        
        m_Window->SwapBuffers();
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
}

void EngineApp::Shutdown()
{
    Texture::CleanUpStaticResources();
    AssetManager::Get().ClearData();
    ImGuiManager::Get().Shutdown();
    m_Window->Shutdown();
}