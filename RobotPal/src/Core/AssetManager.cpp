#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Core/ModelLoader.h"

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

void AssetManager::AddModel(const std::string &name, std::shared_ptr<ModelResource> model)
{
    m_Model[name]=model;
}
std::shared_ptr<ModelResource> AssetManager::GetModel(const std::string &name)
{
    if (m_Model.find(name) != m_Model.end()) return m_Model[name];
    auto model=std::make_shared<ModelResource>();
    ModelLoader::LoadModelData(name, *model);
    m_Model[name] = model;
    return model;
}
void AssetManager::ClearData()
{
    m_Shaders.clear();
    m_Model.clear();
    m_Prefabs.clear();
}
