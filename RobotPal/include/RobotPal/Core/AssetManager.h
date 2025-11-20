#ifndef __ASSETMANAGER_H__
#define __ASSETMANAGER_H__
#include "RobotPal/Core/GraphicsTypes.h"
#include <unordered_map>
#include <flecs.h>

class AssetManager {
public:
    static AssetManager& Get();

    flecs::entity GetPrefab(const std::string& name);
    void AddPrefab(const std::string& name, flecs::entity prefab);
    bool HasPrefab(const std::string& name) const;

    std::shared_ptr<Shader> GetShader(const std::string& filepath);
   
    void AddMesh(const std::string& name, std::shared_ptr<Mesh> mesh);
    std::shared_ptr<Mesh> GetMesh(const std::string& name);

    void ClearData();
private:
    std::unordered_map<std::string, flecs::entity> m_Prefabs;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
};

#endif