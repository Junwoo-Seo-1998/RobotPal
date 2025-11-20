#include "RobotPal/Core/AssetManager.h"

AssetManager &AssetManager::Get()
{
    static AssetManager instance;
    return instance;
}
flecs::entity AssetManager::GetPrefab(const std::string &name)
{
    if (m_Prefabs.find(name) != m_Prefabs.end()) return m_Prefabs[name];
    return flecs::entity::null();
}

void AssetManager::AddPrefab(const std::string &name, flecs::entity prefab)
{
    m_Prefabs[name]=prefab;
}

bool AssetManager::HasPrefab(const std::string &name) const
{
    return m_Prefabs.find(name)!=m_Prefabs.end();
}

std::shared_ptr<Shader> AssetManager::GetShader(const std::string &filepath)
{
    if (m_Shaders.find(filepath) != m_Shaders.end()) return m_Shaders[filepath];
    auto shader = Shader::Create(filepath);
    m_Shaders[filepath] = shader;
    return shader;
}

void AssetManager::AddMesh(const std::string &name, std::shared_ptr<Mesh> mesh)
{
    m_Meshes[name]=mesh;
}

std::shared_ptr<Mesh> AssetManager::GetMesh(const std::string &name)
{
    return m_Meshes[name];
}

void AssetManager::ClearData()
{
    m_Shaders.clear();
    m_Meshes.clear();
    m_Meshes.clear();
}
