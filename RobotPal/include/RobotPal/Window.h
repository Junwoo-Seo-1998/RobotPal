#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <string>

class Window {
public:
    Window(int width, int height, const std::string& title);
    bool Init();
    bool ShouldClose();

    void PollEvents();
    void SwapBuffers();
    void Shutdown();
    void* GetNativeWindow();
    bool IsMinimized();

    int GetWidth() const { return m_LogicalWidth; }
    int GetHeight() const { return m_LogicalHeight; }
private:
    void* m_WindowHandle = nullptr;
    int m_LogicalWidth;
    int m_LogicalHeight;
    int m_PhysicalWidth;
    int m_PhysicalHeight;
    std::string m_Title;
};

#endif