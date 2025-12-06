#include "RobotPal/Core/ModelLoader.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Core/GraphicsTypes.h"
#include "RobotPal/Core/Texture.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <iostream>
#include <memory>
ResourceID LoadTextureFromGLTF(tinygltf::Model& model, int textureIndex, const std::string& modelName, const std::string& suffix) {
    if (textureIndex < 0) return ResourceID(); // 텍스처 없음

    const auto& texture = model.textures[textureIndex];
    const auto& image = model.images[texture.source];

    // 이름 생성: "모델명/텍스처_이미지이름" (중복 방지)
    std::string texName = modelName + "/Texture_" + (image.name.empty() ? std::to_string(texture.source) : image.name) + "_" + suffix;
    
    int channels=image.component;
    TextureFormat format = TextureFormat::RGBA8;
    if (channels == 3) format = TextureFormat::RGB8;
    else if (channels == 4) format = TextureFormat::RGBA8;

    auto tex=std::make_shared<Texture>(image.width, image.height, image.image.data(), format);
    // AssetManager를 통해 생성 및 등록
    return AssetManager::Get().AddRuntimeTexture(tex, texName);
}

bool ModelLoader::LoadModelData(const std::string &path, ModelResource &outResource)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    if (!loader.LoadBinaryFromFile(&model, &err, &warn, path))
        return false;

    // 1. 재질 파싱
    for (const auto &mat : model.materials)
    {
        MaterialData mData;
        mData.name = mat.name;
        // 텍스처 인덱스, 컬러 값 추출..
        {
            auto& d=mat.pbrMetallicRoughness.baseColorFactor;
            mData.baseColorFactor=glm::vec4(d[0], d[1], d[2], d[3]);
        }
        {
            mData.metallicFactor=(float)mat.pbrMetallicRoughness.metallicFactor;
        }

        {
            mData.roughnessFactor=(float)mat.pbrMetallicRoughness.roughnessFactor;
        }

        {
            auto& d=mat.emissiveFactor;
            mData.emissiveFactor=glm::vec3(d[0], d[1], d[2]);
        }
        
        {
            int albedoIdx = mat.pbrMetallicRoughness.baseColorTexture.index;
            mData.baseColorTexture = LoadTextureFromGLTF(model, albedoIdx, path, "Albedo");
        }

        {
            int normalIdx = mat.normalTexture.index;
            mData.normalTexture = LoadTextureFromGLTF(model, normalIdx, path, "Normal");
        }

        {
            int mrIdx = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
            mData.metallicRoughnessTexture = LoadTextureFromGLTF(model, mrIdx, path, "MetallicRoughness");
        }

        {
            int ocIdx = mat.occlusionTexture.index;
            mData.occlusionTexture = LoadTextureFromGLTF(model, ocIdx, path, "Occlusion");
        }

        {
            int emIdx = mat.emissiveTexture.index;
            mData.emissiveTexture = LoadTextureFromGLTF(model, emIdx, path, "Emissive");
        }
        outResource.materials.push_back(mData);
    }

    // 2. 메쉬 파싱 (Primitive -> SubMesh 변환)
    for (const auto &mesh : model.meshes)
    {
        outResource.meshes.push_back(ProcessMesh(model, mesh));
    }

    // 3. 노드 파싱 (재귀 -> 선형 배열)
    const auto &scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

    // 루트 노드가 여러 개일 수 있으므로 가상의 루트를 만들거나,
    // 여기서는 첫 번째 씬의 루트들을 순회하며 처리
    for (int nodeIdx : scene.nodes)
    {
        ProcessNodeRecursive(model, model.nodes[nodeIdx], -1, outResource);
    }

    // 루트 노드 인덱스 설정 (보통 0번이 됨)
    if (!outResource.nodes.empty())
        outResource.rootNodeIndex = 0;

    return true;
}

// 리턴값: 방금 추가된 노드의 인덱스
int ModelLoader::ProcessNodeRecursive(tinygltf::Model &model, const tinygltf::Node &gltfNode, int parentIdx, ModelResource &outResource)
{
    // 1. 노드 데이터 생성 및 벡터 추가
    NodeData newNode;
    using namespace std;
    cout << "Node: " << gltfNode.name << '\n';
    newNode.name = gltfNode.name;
    newNode.meshIndex = gltfNode.mesh; // 메쉬가 없으면 -1
    newNode.parentIndex = parentIdx;

    // Transform 파싱 (Matrix일 수도, TRS일 수도 있음)
    if (gltfNode.matrix.size() == 16)
    {
        // Matrix -> Decompose 필요 (glm::decompose 사용 권장)
        // 여기서는 생략, 직접 TRS 값을 읽는다고 가정
    }
    else
    {
        if (gltfNode.translation.size() == 3)
            newNode.translation = glm::make_vec3(gltfNode.translation.data());
        if (gltfNode.rotation.size() == 4)
        {
            newNode.rotation = glm::eulerAngles(glm::make_quat(gltfNode.rotation.data()));
        }
        if (gltfNode.scale.size() == 3)
            newNode.scale = glm::make_vec3(gltfNode.scale.data());
    }

    // 벡터에 넣고 인덱스 확보 (이 시점에 size가 변하므로 주의)
    int currentIndex = (int)outResource.nodes.size();
    outResource.nodes.push_back(newNode);

    // 2. 자식들 재귀 처리
    for (int childGltfIdx : gltfNode.children)
    {
        int childNodeIdx = ProcessNodeRecursive(model, model.nodes[childGltfIdx], currentIndex, outResource);

        // 중요: 벡터 재할당(resize)이 일어날 수 있으므로 포인터 쓰면 안됨.
        // 방금 넣은 부모 노드에 자식 인덱스 추가
        outResource.nodes[currentIndex].childrenIndices.push_back(childNodeIdx);
    }

    return currentIndex;
}

PrimitiveData ModelLoader::ExtractPrimitiveData(tinygltf::Model &model, const tinygltf::Primitive &primitive)
{
    using namespace std;
    PrimitiveData data;
    data.materialIndex = primitive.material;

    // =============================================================
    // 1. Vertex Processing
    // =============================================================

    // POSITION은 필수이므로 없으면 리턴
    if (primitive.attributes.find("POSITION") == primitive.attributes.end())
    {
        cout << "ERROR: Primitive has no POSITION attribute!\n";
        return data;
    }

    const auto &posAccessor = model.accessors[primitive.attributes.at("POSITION")];
    size_t vertexCount = posAccessor.count;
    data.vertices.resize(vertexCount); // assign 대신 resize 권장

    // 람다 함수: 버퍼 정보 가져오기 (기존 로직 유지하되 안전성 확보)
    auto GetBufferInfo = [&](const std::string &attrName) -> std::pair<const uint8_t *, int>
    {
        if (primitive.attributes.find(attrName) == primitive.attributes.end())
            return {nullptr, 0};

        const auto &accessor = model.accessors[primitive.attributes.at(attrName)];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];

        const uint8_t *ptr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        int stride = accessor.ByteStride(bufferView);

        return {ptr, stride};
    };

    // 포인터 및 정보 획득
    const float *bufPos = nullptr;
    int strPos = 0;
    const float *bufNorm = nullptr;
    int strNorm = 0;
    const float *bufTex = nullptr;
    int strTex = 0;
    const float *bufTan = nullptr;
    int strTan = 0;

    // 애니메이션 관련
    const uint8_t *bufJoints = nullptr;
    int strJoints = 0;
    int typeJoints = 0;
    const uint8_t *bufWeights = nullptr;
    int strWeights = 0;
    int typeWeights = 0;

    // --- 주소 바인딩 ---
    {
        auto info = GetBufferInfo("POSITION");
        bufPos = (const float *)info.first;
        strPos = info.second;
    }
    {
        auto info = GetBufferInfo("NORMAL");
        bufNorm = (const float *)info.first;
        strNorm = info.second;
    }
    {
        auto info = GetBufferInfo("TEXCOORD_0");
        bufTex = (const float *)info.first;
        strTex = info.second;
    }
    {
        auto info = GetBufferInfo("TANGENT");
        bufTan = (const float *)info.first;
        strTan = info.second;
    }

    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
    {
        auto info = GetBufferInfo("JOINTS_0");
        const auto &acc = model.accessors[primitive.attributes.at("JOINTS_0")];
        bufJoints = info.first;
        strJoints = info.second;
        typeJoints = acc.componentType;
    }

    if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
    {
        auto info = GetBufferInfo("WEIGHTS_0");
        const auto &acc = model.accessors[primitive.attributes.at("WEIGHTS_0")];
        bufWeights = info.first;
        strWeights = info.second;
        typeWeights = acc.componentType;
    }

    // --- 데이터 추출 루프 ---
    for (size_t i = 0; i < vertexCount; ++i)
    {
        Vertex &v = data.vertices[i];

        // A. Position
        if (bufPos)
        {
            const float *p = (const float *)((const uint8_t *)bufPos + i * strPos);
            v.Position = glm::vec3(p[0], p[1], p[2]);
        }

        // B. Normal
        if (bufNorm)
        {
            const float *n = (const float *)((const uint8_t *)bufNorm + i * strNorm);
            v.Normal = glm::vec3(n[0], n[1], n[2]);
        }

        // C. TexCoords
        if (bufTex)
        {
            const float *t = (const float *)((const uint8_t *)bufTex + i * strTex);
            v.TexCoords = glm::vec2(t[0], t[1]);
        }

        // D. Tangent
        if (bufTan)
        {
            const float *t = (const float *)((const uint8_t *)bufTan + i * strTan);
            v.Tangent = glm::vec3(t[0], t[1], t[2]);
            // t[3]는 BiTangent 방향(1.0 or -1.0)인데 필요하면 저장하세요.
        }

        // E. Joints (뼈 인덱스)
        if (bufJoints)
        {
            const uint8_t *ptr = bufJoints + i * strJoints;
            if (typeJoints == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                const uint8_t *d = (const uint8_t *)ptr;
                v.BoneIDs = glm::ivec4(d[0], d[1], d[2], d[3]);
            }
            else if (typeJoints == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                const uint16_t *d = (const uint16_t *)ptr;
                v.BoneIDs = glm::ivec4(d[0], d[1], d[2], d[3]);
            }
        }

        // F. Weights (가중치 - 정규화 처리 필수!)
        if (bufWeights)
        {
            const uint8_t *ptr = bufWeights + i * strWeights;
            if (typeWeights == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                const float *d = (const float *)ptr;
                v.Weights = glm::vec4(d[0], d[1], d[2], d[3]);
            }
            else if (typeWeights == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                // 0~65535 값을 0.0~1.0으로 변환
                const uint16_t *d = (const uint16_t *)ptr;
                v.Weights = glm::vec4(d[0], d[1], d[2], d[3]) / 65535.0f;
            }
            else if (typeWeights == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                // 0~255 값을 0.0~1.0으로 변환
                const uint8_t *d = (const uint8_t *)ptr;
                v.Weights = glm::vec4(d[0], d[1], d[2], d[3]) / 255.0f;
            }
        }
    }

    // =============================================================
    // 2. Index Processing
    // =============================================================
    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView &bufferView = model.bufferViews[indexAccessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

        data.indices.reserve(indexAccessor.count);

        const uint8_t *dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;
        int stride = indexAccessor.ByteStride(bufferView); // 보통 인덱스는 tight packed라 stride 무시 가능하지만 안전하게

        for (size_t i = 0; i < indexAccessor.count; ++i)
        {
            const uint8_t *ptr = dataPtr + i * stride;
            uint32_t index = 0;

            // 인덱스 데이터 타입에 따라 분기 (unsigned byte, short, int)
            switch (indexAccessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                index = (uint32_t)(*ptr);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                index = (uint32_t)(*(const uint16_t *)ptr);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                index = *(const uint32_t *)ptr;
                break;
            }
            data.indices.push_back(index);
        }
    }
    else
    {
        // 인덱스가 없는 경우(드물지만), 0, 1, 2... 순서대로 생성해야 그릴 수 있음
        // Primitive Mode가 TRIANGLES라고 가정
        cout << "WARNING: No Indices found. Generating sequential indices.\n";
        for (int i = 0; i < vertexCount; ++i)
        {
            data.indices.push_back(i);
        }
    }

    return data;
}

MeshData ModelLoader::ProcessMesh(tinygltf::Model &model, const tinygltf::Mesh &gltfMesh)
{
    MeshData newMesh;
    using namespace std;
    cout << "Mesh: " << gltfMesh.name << '\n';
    newMesh.name = gltfMesh.name;

    // 여러 Primitive를 하나의 MeshData로 병합
    for (const auto &primitive : gltfMesh.primitives)
    {
        SubMeshInfo subInfo;
        subInfo.indexStart = (uint32_t)newMesh.indices.size(); // 현재까지 쌓인 인덱스 개수가 시작점
        subInfo.defaultMaterialIndex = primitive.material;

        // 1. PrimitiveData 추출 (이전에 작성하신 함수 활용)
        // 주의: 정점 데이터만 뽑아오는 helper 함수로 살짝 수정 필요
        PrimitiveData rawData = ExtractPrimitiveData(model, primitive);

        // 2. 정점 병합 (Vertex Offset 처리 필요)
        uint32_t vertexOffset = (uint32_t)newMesh.vertices.size();

        newMesh.vertices.insert(newMesh.vertices.end(), rawData.vertices.begin(), rawData.vertices.end());

        // 3. 인덱스 병합 (중요: Vertex Offset을 더해줘야 함!)
        for (uint32_t idx : rawData.indices)
        {
            newMesh.indices.push_back(idx + vertexOffset);
        }

        subInfo.indexCount = (uint32_t)rawData.indices.size();
        newMesh.subMeshes.push_back(subInfo);
    }

    return newMesh;
}

flecs::entity PrefabFactory::CreatePrefab(flecs::world &ecs, const std::string &name, const ModelResource &res)
{
    // 1. 루트 프리팹 생성
    auto rootPrefab = ecs.prefab(name.c_str()); // .prefab()은 자동으로 flecs::Prefab 태그를 붙임

    rootPrefab//.add<Position, World>()
    .add<Position, Local>()
    //.add<Rotation, World>()
    .add<Rotation, Local>()
    //.add<Scale, World>()
    .add<Scale, Local>()
    .add<TransformMatrix, World>()
    .add<TransformMatrix, Local>();

    // 2. 재귀적으로 자식 프리팹 생성
    CreateNodesRecursive(ecs, rootPrefab, res, res.rootNodeIndex);

    return rootPrefab;
}

void PrefabFactory::CreateNodesRecursive(flecs::world &ecs, flecs::entity parentPrefab, const ModelResource &res, int nodeIdx)
{
    const NodeData &node = res.nodes[nodeIdx];

    // 자식 프리팹 생성 (부모가 프리팹이면 자식도 자동으로 프리팹 성격 가짐)
    auto entity = ecs.entity(node.name.c_str())
                      .child_of(parentPrefab); // 계층 구조 연결

    entity//.add<Position, World>()
    .add<Position, Local>()
    //.add<Rotation, World>()
    .add<Rotation, Local>()
    //.add<Scale, World>()
    .add<Scale, Local>()
    .add<TransformMatrix, World>()
    .add<TransformMatrix, Local>();

    // Transform 설정 (초기값)
    // 쿼터니언 -> 오일러 변환은 여기서 수행
    //entity.set<Transform>({node.translation, glm::degrees(glm::eulerAngles(node.rotation)), node.scale});
    entity.set<Position, Local>({node.translation});
    entity.set<Rotation, Local>({node.rotation});
    entity.set<Scale, Local>({node.scale});

    // 렌더링 데이터 연결
    if (node.meshIndex >= 0)
    {
        auto& meshData=res.meshes[node.meshIndex];
        entity.set<MeshFilter>({meshData.uniqueID});
        // 실제 데이터는 복사하지 않고 ID(인덱스)만 참조!
        //entity.set<MeshFilter>({(ResourceID)node.meshIndex});

        // 재질 정보 등도 여기서 설정
        // (2) MeshRenderer: 어떤 메터리얼들을 쓰는가?
        MeshRenderer renderer;
        renderer.materials.reserve(meshData.subMeshes.size());

        // 메쉬의 서브메쉬 개수만큼 돌면서 메터리얼 ID 수집
        for (const auto& subMesh : meshData.subMeshes) {
            // AssetManager에서 미리 변환해둔 ID를 가져옴
            renderer.materials.push_back(res.materials[subMesh.defaultMaterialIndex].uniqueID);
        }

        // 컴포넌트 부착 (std::vector 복사 발생하지만, 초기화 시 1회라 괜찮음)
        entity.set<MeshRenderer>(renderer);
    }

    for (int childIdx : node.childrenIndices)
    {
        CreateNodesRecursive(ecs, entity, res, childIdx);
    }
}