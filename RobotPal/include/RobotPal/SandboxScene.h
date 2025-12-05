#ifndef __SANDBOXSCENE_H__
#define __SANDBOXSCENE_H__
#include "RobotPal/Scene.h"
#include "glm/glm.hpp"
#include <memory>
#include "RobotPal/Core/GraphicsTypes.h"
#include "RobotPal/GlobalComponents.h"
class VertexArray;
class Shader;

class NetworkTransport;

class SandboxScene : public Scene
{
public:
    using Scene::Scene;

    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnExit() override;
    void OnImGuiRender() override;

private:
};

#endif