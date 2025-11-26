#ifndef __SANDBOXSCENE_H__
#define __SANDBOXSCENE_H__
#include "RobotPal/Scene.h"
#include "glm/glm.hpp"
#include <memory>
#include "RobotPal/Core/GraphicsTypes.h"
#include "RobotPal/GlobalComponents.h"
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
    // 재귀 렌더링을 위한 헬퍼 함수
    void RenderNode(const NodeData& node, const glm::mat4& parentTransform, const ModelResource& modelRes);

private:
    Entity m_Center;
    
    // 쉐이더
    std::shared_ptr<Shader> m_CubeShader;
    
    // 모델의 모든 메쉬에 대한 VAO 목록 (인덱스로 접근)
    std::vector<std::shared_ptr<VertexArray>> m_MeshVAOs;

    glm::vec4 m_CubeColor = {1.0f, 1.0f, 1.0f, 1.0f};
    WindowData m_WindowSize{}; // 윈도우 크기 저장용
};

#endif