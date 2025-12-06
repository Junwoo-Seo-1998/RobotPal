#ifndef __SCENEMANAGER_H__
#define __SCENEMANAGER_H__
#include "Scene.h"
#include <memory>

class SceneManager {
public:
    SceneManager(flecs::world& world);

    template<typename T>
    void LoadScene() {
        m_NextScene = std::make_shared<T>(m_World);
    }

    void OnUpdate(float dt);

    void OnImGuiRender();

private:
    flecs::world& m_World;
    std::shared_ptr<Scene> m_ActiveScene;
    std::shared_ptr<Scene> m_NextScene;
};

#endif