#ifndef __ENGINEAPP_H__
#define __ENGINEAPP_H__
#include <memory>
#include <glm/glm.hpp>
#include <flecs.h>

class Window;
class VertexArray;
class Shader;
class SceneManager;

class EngineApp
{
public:
    void Run();


private:
    void Init();
    void MainLoop();
    void Shutdown();
    
    float m_LastFrameTime;
    std::shared_ptr<SceneManager> m_SceneManager;

    std::shared_ptr<Window> m_Window;
    std::shared_ptr<VertexArray> m_CubeVA;
    std::shared_ptr<Shader> m_CubeShader;
    glm::vec4 m_CubeColor;

    flecs::world m_World;
};

#endif