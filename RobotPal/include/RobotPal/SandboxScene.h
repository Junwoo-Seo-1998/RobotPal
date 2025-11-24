#ifndef __SANDBOXSCENE_H__
#define __SANDBOXSCENE_H__
#include "RobotPal/Scene.h"
#include "glm/glm.hpp"
#include <memory>

class VertexArray;
class Shader;

class SandboxScene : public Scene {
public:
    using Scene::Scene;

    void OnEnter() override;

    void OnUpdate(float dt) override;

    void OnExit() override;

    void OnImGuiRender() override;

private:
    Entity m_Camera;
    std::shared_ptr<VertexArray> m_CubeVA;
    std::shared_ptr<Shader> m_CubeShader;
    glm::vec4 m_CubeColor;
};

#endif