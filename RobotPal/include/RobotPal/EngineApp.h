#ifndef __ENGINEAPP_H__
#define __ENGINEAPP_H__
#include <memory>
#include <glm/glm.hpp>

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
    std::shared_ptr<VertexArray> m_CubeVA;
    std::shared_ptr<Shader> m_CubeShader;
    glm::vec4 m_CubeColor;
};

#endif