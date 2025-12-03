#include "RobotPal/Window.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>
#include "imgui_impl_glfw.h" // GetContentScaleForMonitor를 위해 필요

Window::Window(int width, int height, const std::string &title)
:m_LogicalWidth(width), m_LogicalHeight(height), m_PhysicalWidth(width), m_PhysicalHeight(height), m_Title(title)
{

}

bool Window::Init()
{
    GLFWwindow* window;
    if (!glfwInit())
        return false;

    /* Create a windowed mode window and its OpenGL context */
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_SAMPLES, 8);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    m_PhysicalWidth=(int)(1280 * main_scale);
    m_PhysicalHeight=(int)(800 * main_scale);
    // Create window with graphics context
    window = glfwCreateWindow(m_PhysicalWidth, m_PhysicalHeight, m_Title.c_str(), NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    gladLoadGLES2(glfwGetProcAddress);
    glfwSwapInterval(1); // Enable vsync

    m_WindowHandle=window;
    return true;
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose((GLFWwindow*)m_WindowHandle);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::SwapBuffers()
{
    glfwSwapBuffers((GLFWwindow*)m_WindowHandle);
}

void Window::Shutdown()
{
    glfwDestroyWindow((GLFWwindow*)m_WindowHandle);
    glfwTerminate();
}

void *Window::GetNativeWindow()
{
    return m_WindowHandle;
}

bool Window::IsMinimized() 
{
    return glfwGetWindowAttrib((GLFWwindow*)m_WindowHandle, GLFW_ICONIFIED) != 0;
}
