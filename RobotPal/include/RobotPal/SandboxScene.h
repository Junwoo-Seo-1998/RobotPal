#ifndef __SANDBOXSCENE_H__
#define __SANDBOXSCENE_H__
#include "RobotPal/Scene.h"
#include "glm/glm.hpp"
#include <memory>
#include "RobotPal/Core/GraphicsTypes.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/StreamingManager.h"
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
    std::shared_ptr<StreamingManager> m_StreamingManager;
};

#endif