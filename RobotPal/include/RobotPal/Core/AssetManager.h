#ifndef __ASSETMANAGER_H__
#define __ASSETMANAGER_H__
#include "RobotPal/Core/GraphicsTypes.h"
#include "RobotPal/Core/ResourceID.h"
#include "RobotPal/Core/Texture.h"
#include <unordered_map>
#include <flecs.h>

class AssetManager {
public:
    static AssetManager& Get();

    ResourceID LoadTextureHDR(const std::string& path);
    // IBL 베이커가 만든 텍스처를 등록하고 ID를 발급받는 함수
    ResourceID AddRuntimeTextureHDR(std::shared_ptr<Texture> texture, const std::string& name);
    ResourceID AddRuntimeTexture(std::shared_ptr<Texture> texture, const std::string& name);
    flecs::entity GetPrefab(flecs::world &ecs, const std::string& name);

    std::shared_ptr<Shader> GetShader(const std::string& filepath);
   
    void AddModel(const std::string& name, std::shared_ptr<ModelResource> model);
    std::shared_ptr<ModelResource> GetModel(const std::string& name);

    std::shared_ptr<Texture> GetTextureHDR(ResourceID id);
    std::shared_ptr<Texture> GetTexture(ResourceID id);
    const MaterialData* GetMaterial(ResourceID id);
    const MeshData* GetMesh(ResourceID id);

    void ClearData();
private:
    std::unordered_map<ResourceID, std::shared_ptr<Texture>> m_TextureHDR;
    std::unordered_map<ResourceID, std::shared_ptr<Texture>> m_Texture;
    std::unordered_map<std::string, flecs::entity> m_Prefabs;
    std::unordered_map<std::string, std::shared_ptr<ModelResource>> m_Model;
    std::unordered_map<ResourceID, MaterialData*> m_Material;
    std::unordered_map<ResourceID, MeshData*> m_Mesh;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
};

#endif