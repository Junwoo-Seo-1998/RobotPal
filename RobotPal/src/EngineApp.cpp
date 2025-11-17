#include "RobotPal/EngineApp.h"
#include "RobotPal/Window.h"
#include "RobotPal/ImGuiManager.h"
#include "RobotPal/Util/emscripten_mainloop.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>
#include "RobotPal/Buffer.h"
void EngineApp::Run()
{
    Init();
    MainLoop();
    Shutdown();
}

void EngineApp::Init()
{
    m_Window=std::make_unique<Window>(1280, 720, "RobotPal");
    m_Window->Init();
    ImGuiManager::Get().Init(m_Window->GetNativeWindow());
}

void EngineApp::MainLoop()
{
    //TODO: rm later
    bool test=true;
    if(test)
    {

        float vertices[] = {
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
            0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        };
        auto vertexBuffer = std::make_shared<VertexBuffer>(vertices, sizeof(vertices));

        BufferLayout layout = {
            { DataType::Float3, "a_Position" }, // Location 0
            { DataType::Float3, "a_Normal" },   // Location 1
            { DataType::Float2, "a_TexCoord" }  // Location 2
        };
        vertexBuffer->SetLayout(layout);

    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!m_Window->ShouldClose())
#endif
    {
        m_Window->PollEvents();
        if (m_Window->IsMinimized())
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        
        ImGuiManager::Get().NewFrame();

        //draw gui
        {
            //test
            ImGui::Begin("Test");
            ImGui::Text("HELLO TEST");
            ImGui::End();
        }

        ImGuiManager::Get().PrepareRender();

        int display_w, display_h;
        glfwGetFramebufferSize((GLFWwindow*)m_Window->GetNativeWindow(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGuiManager::Get().Render(m_Window->GetNativeWindow());
        m_Window->SwapBuffers();
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
}

void EngineApp::Shutdown()
{
    ImGuiManager::Get().Shutdown();
    m_Window->Shutdown();
}