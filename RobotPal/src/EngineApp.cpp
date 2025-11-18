#include "RobotPal/EngineApp.h"
#include "RobotPal/Window.h"
#include "RobotPal/ImGuiManager.h"
#include "RobotPal/Util/emscripten_mainloop.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include "RobotPal/VertexArray.h"
#include "RobotPal/Buffer.h"
#include "RobotPal/Shader.h"

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

    // Create triangle
    m_TriangleVA = VertexArray::Create();

    float vertices[] = {
        // Positions
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };
    auto vertexBuffer = std::make_shared<VertexBuffer>(vertices, sizeof(vertices));

    BufferLayout layout = {
        { DataType::Float3, "a_Position" },
    };
    vertexBuffer->SetLayout(layout);
    m_TriangleVA->AddVertexBuffer(vertexBuffer);

    uint32_t indices[] = { 0, 1, 2 };
    auto indexBuffer = std::make_shared<IndexBuffer>(indices, sizeof(indices) / sizeof(uint32_t));
    m_TriangleVA->SetIndexBuffer(indexBuffer);

    std::string vertexSrc = R"(#version 300 es
        precision mediump float;
        layout(location = 0) in vec3 a_Position;
        void main()
        {
            gl_Position = vec4(a_Position, 1.0);
        }
    )";

    std::string fragmentSrc = R"(#version 300 es
        precision mediump float;
        layout(location = 0) out vec4 color;
        void main()
        {
            color = vec4(0.8, 0.2, 0.3, 1.0);
        }
    )";

    m_TriangleShader = Shader::CreateFromSource("SimpleTriangle", vertexSrc, fragmentSrc);
}

void EngineApp::MainLoop()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
#ifdef __EMSCRIPTEN__
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
        
        // Clear the screen
        int display_w, display_h;
        glfwGetFramebufferSize((GLFWwindow*)m_Window->GetNativeWindow(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the triangle
        m_TriangleShader->Bind();
        m_TriangleVA->Bind();
        glDrawElements(GL_TRIANGLES, m_TriangleVA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);

        // Start the Dear ImGui frame
        ImGuiManager::Get().NewFrame();

        //draw gui
        {
            ImGui::Begin("Test");
            ImGui::Text("HELLO TEST");
            ImGui::End();
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
    m_TriangleVA = nullptr;
    m_TriangleShader = nullptr;
    ImGuiManager::Get().Shutdown();
    m_Window->Shutdown();
}