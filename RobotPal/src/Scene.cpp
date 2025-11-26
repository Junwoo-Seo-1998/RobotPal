#include "RobotPal/Scene.h"
#include "RobotPal/Components/Components.h"
Entity Scene::CreateEntity(const std::string &name)
{
    flecs::entity e = m_World.entity(name.c_str());
    e//.add<Position, World>()
     .add<Position, Local>()
     //.add<Rotation, World>()
     .add<Rotation, Local>()
     //.add<Scale, World>()
     .add<Scale, Local>()
     .add<TransformMatrix, World>()
     .add<TransformMatrix, Local>();
    
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
