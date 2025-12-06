#ifndef __RENDERSYSTEMMODULE_H__
#define __RENDERSYSTEMMODULE_H__
#include <flecs.h>
#include <memory>
#include "RobotPal/Shader.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/VertexArray.h"
#include "RobotPal/Core/Texture.h"
#include "RobotPal/Core/Framebuffer.h"
struct RenderSystemModule{
public:
    RenderSystemModule(flecs::world &world);

private:
    flecs::world &world;

    void RegisterSystem();
    void InitSkybox();
    void RenderScene(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& viewPos);
    WindowData m_WindowSize{}; // 윈도우 크기 저장용
    std::shared_ptr<Shader> m_SimpleShader;
    flecs::query<const MeshFilter, const MeshRenderer, const TransformMatrix> renderQuery;
    std::shared_ptr<Shader> m_SkyboxShader;
    std::shared_ptr<VertexArray> m_SkyboxVAO;


    std::shared_ptr<Framebuffer> m_CubemapFBO;
    std::shared_ptr<Texture> m_CubemapTexture;
    std::shared_ptr<Shader> m_FishEyeShader;

    std::shared_ptr<VertexArray> m_QuadVAO;
};

#endif