#include "RobotPal/EngineApp.h"
#include "RobotPal/Window.h"
#include "RobotPal/ImGuiManager.h"
//#include "RobotPal/Util/emscripten_mainloop.h

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__  
#include <emscripten.h>
#include <functional>
// To give the Emscripten main loop access to the EngineApp instance
static std::function<void()> MainLoopForEmscripten;
void emscripten_main_loop()
{                                                                                                                                                                        
    MainLoopForEmscripten();
}       
#endif

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
#ifdef __EMSCRIPTEN__  
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    MainLoopForEmscripten=[&]()
    {
        MainLoopIteration();
    };
    emscripten_set_main_loop(emscripten_main_loop, 0, true);
#else
    while (!m_Window->ShouldClose())
    {
        MainLoopIteration();
    }
#endif
}

void EngineApp::MainLoopIteration()
{
    //TODO: rm later
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    {
        m_Window->PollEvents();
        if (m_Window->IsMinimized())
        {
            ImGui_ImplGlfw_Sleep(10);
            return;
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
}

void EngineApp::Shutdown()
{
    ImGuiManager::Get().Shutdown();
    m_Window->Shutdown();
}