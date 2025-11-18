#ifndef __ENGINEAPP_H__
#define __ENGINEAPP_H__
#include <memory>

class Window;
class VertexArray;
class Shader;

class EngineApp
{
public:
    void Run();
private:
    void Init();
    void MainLoop();
    void Shutdown();
    
    std::shared_ptr<Window> m_Window;
    std::shared_ptr<VertexArray> m_TriangleVA;
    std::shared_ptr<Shader> m_TriangleShader;
};

#endif