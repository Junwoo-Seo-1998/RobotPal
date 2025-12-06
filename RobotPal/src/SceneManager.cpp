#include "RobotPal/SceneManager.h"

SceneManager::SceneManager(flecs::world &world)
    : m_World(world) {}

void SceneManager::OnUpdate(float dt)
{
    if (m_NextScene)
    {
        if (m_ActiveScene)
        {
            m_ActiveScene->OnExit();
            m_ActiveScene->CleanupRoot();
        }

        m_ActiveScene = m_NextScene;
        m_NextScene = nullptr;

        m_ActiveScene->InitRoot();
        m_ActiveScene->OnEnter();
    }

    if (m_ActiveScene)
    {
        m_ActiveScene->OnUpdate(dt);
    }
}

void SceneManager::OnImGuiRender()
{
    if (m_ActiveScene)
        m_ActiveScene->OnImGuiRender();
}