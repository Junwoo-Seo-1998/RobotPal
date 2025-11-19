#include "RobotPal/Scene.h"

Entity Scene::CreateEntity(const std::string &name)
{
    flecs::entity e = m_World.entity(name.c_str());

    if (m_SceneRoot) 
    {
        e.child_of(m_SceneRoot); 
    }
    return Entity(e);
}
flecs::world &Scene::GetWorld()
{
    return m_World;
}

flecs::entity Scene::GetSceneRoot()
{
    return m_SceneRoot;
}
