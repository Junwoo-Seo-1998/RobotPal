#ifndef __ENGINEAPP_H__
#define __ENGINEAPP_H__
#include <memory>
class Window;
class EngineApp
{
public:
    void Run();
private:
    void Init();
    void MainLoop();
    void Shutdown();
    std::shared_ptr<Window> m_Window;
};

#endif