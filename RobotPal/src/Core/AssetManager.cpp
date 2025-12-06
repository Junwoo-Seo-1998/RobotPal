#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Core/ModelLoader.h"
#include "RobotPal/Core/ResourceID.h"
#include <iostream>
#include "stb_image.h"


AssetManager &AssetManager::Get()
{
    static AssetManager instance;
    return instance;
}

ResourceID AssetManager::LoadTextureHDR(const std::string &path)
{
    ResourceID id(path);
    if (m_TextureHDR.find(id) != m_TextureHDR.end()) return id;

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 4);
    
    if (data) {
        // Texture 클래스 재활용: RGB16F 포맷으로 생성
        auto texture = std::make_shared<Texture>(width, height, data, TextureFormat::RGBA16F);
        stbi_image_free(data);
        
        // ID 관리 로직 (기존 GetID 등 활용)
        m_TextureHDR[id] = texture; // m_Textures 맵이 있다고 가정
        return id;
    }
    std::cout << "Failed to load HDR image." << std::endl;
    return ResourceID(); // 실패
}

ResourceID AssetManager::AddRuntimeTextureHDR(std::shared_ptr<Texture> texture, const std::string &name)
{
    ResourceID id(name);
    m_TextureHDR[id] = texture;
    return id;
}

ResourceID AssetManager::AddRuntimeTexture(std::shared_ptr<Texture> texture, const std::string &name)
{
    ResourceID id(name);
    m_Texture[id] = texture;
    return id;
}

flecs::entity AssetManager::GetPrefab(flecs::world &ecs, const std::string &name)
{
    if (m_Prefabs.find(name) != m_Prefabs.end()) return m_Prefabs[name];

    auto model=GetModel(name);
    flecs::entity prefab=PrefabFactory::CreatePrefab(ecs, name, *model);
    m_Prefabs[name]=prefab;
    return prefab;
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


    // 3. 모든 메쉬에 대해 VAO 생성 (한 번만 수행)

    // Vertex 구조체와 일치하는 레이아웃
    const BufferLayout layout = {
        {DataType::Float3, "a_Position"},
        {DataType::Float3, "a_Normal"},
        {DataType::Float2, "a_TexCoord"},
        {DataType::Float3, "a_Tangent"},
        {DataType::Int4,   "a_BoneIDs"},
        {DataType::Float4, "a_Weights"}
    };

    for (int i=0; i<model->materials.size(); ++i)
    {
        MaterialData& mat=model->materials[i];

        // 이름이 없으면 임시 이름 생성
        std::string matName = mat.name.empty() ? "Mat_" + std::to_string(i) : mat.name;
        // 고유 ID 생성 (모델명/재질명 조합 추천)
        std::string uniqueIDStr = name + "/Materials/" + matName;

        ResourceID id(uniqueIDStr);
        mat.uniqueID = id;
        m_Material[id]=&mat;


    }

    for (int i = 0; i < model->meshes.size(); ++i) {
        MeshData& mesh = model->meshes[i];

        // [이전 방식 - 위험]
        // std::string idStr = path + "_Mesh_" + std::to_string(i);

        // [개선된 방식 - 안전] 메쉬 이름을 사용!
        std::string subName = mesh.name;
        
        // 이름이 비어있을 경우를 대비한 안전장치 (익명 메쉬)
        if (subName.empty()) {
            subName = "UnnamedMesh_" + std::to_string(i);
        }

        // 경로 + 메쉬이름 조합 (예: "Assets/Tank.glb/Turret")
        std::string uniqueIDStr = name + "/" + subName;
        
        // ID 발급
        ResourceID id(uniqueIDStr);

        // 데이터에 ID 부착
        mesh.uniqueID = id; 
        
        // 캐시에 등록
        m_Mesh[id] = &mesh;
        
        // GPU 업로드
        //todo:
        //UploadMeshToGPU(mesh);
        
        auto va = VertexArray::Create();
        
        // VBO
        auto vb = std::make_shared<VertexBuffer>(
            mesh.vertices.data(), 
            mesh.vertices.size() * sizeof(Vertex)
        );
        vb->SetLayout(layout);
        va->AddVertexBuffer(vb);

        // IBO
        auto ib = IndexBuffer::Create(mesh.indices.data(), mesh.indices.size());
        va->SetIndexBuffer(ib);

        mesh.vao=va; // 저장
    };

    return model;
}

std::shared_ptr<Texture> AssetManager::GetTextureHDR(ResourceID id)
{
    return m_TextureHDR[id];
}

std::shared_ptr<Texture> AssetManager::GetTexture(ResourceID id)
{
    return m_Texture[id];
}

const MaterialData *AssetManager::GetMaterial(ResourceID id)
{
    return m_Material[id];
}
const MeshData *AssetManager::GetMesh(ResourceID id)
{
    return m_Mesh[id];
}

void AssetManager::ClearData()
{
    m_TextureHDR.clear();
    m_Texture.clear();
    m_Shaders.clear();
    m_Model.clear();
    m_Mesh.clear();
    m_Prefabs.clear();
}
