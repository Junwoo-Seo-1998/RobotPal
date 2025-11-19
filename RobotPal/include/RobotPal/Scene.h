#ifndef __SCENE_H__
#define __SCENE_H__
#include <flecs.h>
#include "RobotPal/Entity.h"
#include <string>

class Scene {
public:
    explicit Scene(flecs::world& world) : m_World(world) {}
    virtual ~Scene() = default;

    virtual void OnEnter() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnExit() {}
    virtual void OnImGuiRender() {}

    Entity CreateEntity(const std::string& name = "");

    flecs::world& GetWorld();
    flecs::entity GetSceneRoot();

private:
    friend class SceneManager; 

    void InitRoot() {
        m_SceneRoot = m_World.entity(); 
    }

    void CleanupRoot() {
        if (m_SceneRoot.is_valid()) {
            m_SceneRoot.destruct(); 
        }
    }

protected:
    flecs::world& m_World;
    flecs::entity m_SceneRoot;
};

#endif