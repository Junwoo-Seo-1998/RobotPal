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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    m_CubeColor = glm::vec4(0.2f, 0.3f, 0.8f, 1.0f);

    // Create cube
    m_CubeVA = VertexArray::Create();

    float vertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
    auto vertexBuffer = std::make_shared<VertexBuffer>(vertices, sizeof(vertices));

    BufferLayout layout = {
        { DataType::Float3, "a_Position" },
        { DataType::Float3, "a_Normal" }
    };
    vertexBuffer->SetLayout(layout);
    m_CubeVA->AddVertexBuffer(vertexBuffer);

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
            vec3 lightPos = vec3(1.2, 1.0, 2.0);
            
            // Ambient
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

            // Diffuse
            vec3 norm = normalize(v_Normal);
            vec3 lightDir = normalize(lightPos - v_FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(u_ViewPos - v_FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);

            vec3 result = (ambient + diffuse + specular) * u_Color.rgb;
            color = vec4(result, u_Color.a);
        }
    )";

    m_CubeShader = Shader::CreateFromSource("PhongCube", vertexSrc, fragmentSrc);
}

void EngineApp::MainLoop()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glEnable(GL_DEPTH_TEST);
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create transformations
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 3.0f);
        view  = glm::translate(glm::mat4(1.0f), -viewPos);
        projection = glm::perspective(glm::radians(45.0f), (float)m_Window->GetWidth() / (float)m_Window->GetHeight(), 0.1f, 100.0f);

        // Draw the cube
        m_CubeShader->Bind();
        m_CubeShader->SetMat4("u_Model", model);
        m_CubeShader->SetMat4("u_View", view);
        m_CubeShader->SetMat4("u_Projection", projection);
        m_CubeShader->SetFloat4("u_Color", m_CubeColor);
        m_CubeShader->SetFloat3("u_ViewPos", viewPos);
        
        m_CubeVA->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Start the Dear ImGui frame
        ImGuiManager::Get().NewFrame();

        //draw gui
        {
            ImGui::Begin("Cube Color");
            ImGui::ColorEdit4("Color", &m_CubeColor.x);
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
    m_CubeVA = nullptr;
    m_CubeShader = nullptr;
    ImGuiManager::Get().Shutdown();
    m_Window->Shutdown();
}