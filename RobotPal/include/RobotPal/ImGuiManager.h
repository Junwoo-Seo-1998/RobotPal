#ifndef __IMGUIMANAGER_H__
#define __IMGUIMANAGER_H__

class ImGuiManager
{
public:
    static ImGuiManager& Get();

    void Init(void* window);
    void NewFrame();
    void PrepareRender();
    void Render(void* window);
    void Shutdown();
    
};

#endif