#ifndef __RENDERSYSTEMMODULE_H__
#define __RENDERSYSTEMMODULE_H__
#include <flecs.h>
#include <memory>
#include "RobotPal/Shader.h"
#include "RobotPal/GlobalComponents.h"
#include "RobotPal/Components/Components.h"
struct RenderSystemModule{
public:
    RenderSystemModule(flecs::world &world);

private:
    void RegisterSystem(flecs::world& world);
    WindowData m_WindowSize{}; // 윈도우 크기 저장용
    std::shared_ptr<Shader> m_SimpleShader;
    flecs::query<const MeshFilter, const MeshRenderer, const TransformMatrix> renderQuery;
};

#endif