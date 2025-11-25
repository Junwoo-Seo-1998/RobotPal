#include "RobotPal/Core/ModelLoader.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Core/GraphicsTypes.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
flecs::entity ModelLoader::LoadModel(Scene *scene, const std::string &filepath)
{
    using namespace std;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
    if (!ret) {
        return flecs::entity::null();
    }

    Entity root = scene->CreateEntity(filepath);
    flecs::entity rootHandle = root.GetHandle();

    rootHandle.add(flecs::Prefab);

    const auto& gltfScene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
    for (int nodeIdx : gltfScene.nodes) {
        cout<<"size: " << gltfScene.nodes.size() <<"\n";
        ProcessNode(scene, rootHandle, model, model.nodes[nodeIdx]);
    }

    AssetManager::Get().AddPrefab(filepath, rootHandle);

    return rootHandle;
}

void ModelLoader::ProcessNode(Scene *scene, flecs::entity parent, tinygltf::Model &model, tinygltf::Node &node)
{
    using namespace std;
    cout<<"name: " << node.name <<"\n";

    if(node.mesh>=0)
    {
        const tinygltf::Mesh &mesh = model.meshes[node.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); ++i) {
            string uniqueResourceName = node.name + "_Primitive_" + to_string(i);
            ProcessMesh(model, mesh.primitives[i], uniqueResourceName);
        }
    }

    for (int childIndex : node.children) 
    {
        ProcessNode(scene, parent, model, model.nodes[childIndex]);
    }
}

PrimitiveData ModelLoader::ProcessMesh(tinygltf::Model &model, const tinygltf::Primitive &primitive, const std::string &name)
{
    using namespace std;
    cout<<"primitive: " << name <<"\n";

    PrimitiveData data;

    // 1. 버텍스 개수 파악 (POSITION은 무조건 존재한다고 가정)
    const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
    size_t vertexCount = posAccessor.count;
    cout<<"primitive - vertexCount: " << vertexCount <<"\n";
    
    data.vertices.assign(vertexCount, {});
    data.materialIndex = primitive.material;

    auto GetBufferInfo = [&](const std::string& attrName) -> std::pair<const void*, int> {
        // 1. 속성이 존재하는지 확인
        if (primitive.attributes.find(attrName) == primitive.attributes.end()) {
            return { nullptr, 0 };
        }

        const int accessorIdx = primitive.attributes.at(attrName);
        const auto& accessor = model.accessors[accessorIdx];

        // [중요 1] BufferView가 유효한지 체크 (-1일 수 있음!)
        if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
            cout << "WARNING: " << attrName << " has invalid bufferView index: " << accessor.bufferView << "\n";
            return { nullptr, 0 };
        }

        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        // [중요 2] 데이터 범위 체크 (오버플로우 방지)
        size_t totalOffset = bufferView.byteOffset + accessor.byteOffset;
        if (totalOffset >= buffer.data.size()) {
            cout << "ERROR: " << attrName << " data offset is out of bounds!\n";
            return { nullptr, 0 };
        }

        // [중요 3] &buffer.data[...] 대신 .data() + offset 사용 (빈 벡터일 때 안전)
        const void* ptr = buffer.data.data() + totalOffset;

        // Stride 계산
        int stride = accessor.ByteStride(bufferView);
        
        // [중요 4] 만약 Stride가 0이 나오면(Tightly Packed), 데이터 타입 크기로 강제 설정 필요할 수 있음
        // 하지만 tinygltf의 ByteStride는 보통 0일 때 자동 계산해줍니다. 
        // 혹시 모르니 -1이 나오면 에러 처리.
        if (stride < 0) {
            cout << "ERROR: Invalid stride for " << attrName << "\n";
            return { nullptr, 0 };
        }

        return { ptr, stride };
    };

    // 포인터들을 저장할 변수들
    const float* bufferPos = nullptr;
    const float* bufferNorm = nullptr;
    const float* bufferTex = nullptr;
    const float* bufferTan = nullptr;
    
    // 조인트/웨이트는 데이터 타입이 다양해서(byte, short, float) void*로 받음
    const void* bufferJoints = nullptr; 
    const void* bufferWeights = nullptr;

    int stridePos = 0, strideNorm = 0, strideTex = 0, strideTan = 0;
    int strideJoints = 0, strideWeights = 0;
    int typeJoints = 0, typeWeights = 0; // 데이터 타입 저장 (GL_FLOAT, GL_UNSIGNED_SHORT 등)

    // 각 속성 주소 획득
    { auto info = GetBufferInfo("POSITION");   bufferPos = (const float*)info.first; stridePos = info.second; }
    { auto info = GetBufferInfo("NORMAL");     bufferNorm = (const float*)info.first; strideNorm = info.second; }
    { auto info = GetBufferInfo("TEXCOORD_0"); bufferTex = (const float*)info.first; strideTex = info.second; }
    { auto info = GetBufferInfo("TANGENT");    bufferTan = (const float*)info.first; strideTan = info.second; }

    // 애니메이션 속성은 타입 확인이 필요해서 별도 처리
    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("JOINTS_0")];
        auto info = GetBufferInfo("JOINTS_0");
        bufferJoints = info.first;
        strideJoints = info.second;
        typeJoints = accessor.componentType; // 5121(ubyte) or 5123(ushort)
    }
    if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
        const auto& accessor = model.accessors[primitive.attributes.at("WEIGHTS_0")];
        auto info = GetBufferInfo("WEIGHTS_0");
        bufferWeights = info.first;
        strideWeights = info.second;
        typeWeights = accessor.componentType; // 보통 FLOAT
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        Vertex& v = data.vertices[i];

        // A. Position (Vec3)
        // (char*)로 캐스팅 후 stride만큼 이동 -> 다시 (float*)로 캐스팅 -> 값 읽기
        // 이렇게 해야 stride가 0이든 12든 32든 정확하게 읽음
        if (bufferPos) {
            const float* p = (const float*)((const char*)bufferPos + i * stridePos);
            v.Position = glm::vec3(p[0], p[1], p[2]);
        }

        // B. Normal (Vec3)
        if (bufferNorm) {
            const float* n = (const float*)((const char*)bufferNorm + i * strideNorm);
            v.Normal = glm::vec3(n[0], n[1], n[2]);
        }

        // C. TexCoords (Vec2)
        if (bufferTex) {
            const float* t = (const float*)((const char*)bufferTex + i * strideTex);
            v.TexCoords = glm::vec2(t[0], t[1]);
        }

        // D. Tangent (Vec3 or Vec4)
        if (bufferTan) {
            const float* t = (const float*)((const char*)bufferTan + i * strideTan);
            v.Tangent = glm::vec3(t[0], t[1], t[2]);
        }

        // E. Bone IDs (주의: 데이터 타입이 다양함!)
        if (bufferJoints) {
            const char* ptr = (const char*)bufferJoints + i * strideJoints;
            
            // GLTF는 용량 최적화를 위해 뼈 ID를 unsigned byte나 short로 저장함
            if (typeJoints == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* d = (const uint8_t*)ptr;
                v.BoneIDs = glm::ivec4(d[0], d[1], d[2], d[3]);
            } 
            else if (typeJoints == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* d = (const uint16_t*)ptr;
                v.BoneIDs = glm::ivec4(d[0], d[1], d[2], d[3]);
            }
        } else {
            v.BoneIDs = glm::ivec4(-1); // 뼈 없음
        }

        // F. Weights (Vec4)
        if (bufferWeights) {
            const char* ptr = (const char*)bufferWeights + i * strideWeights;
            
            if (typeWeights == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* d = (const float*)ptr;
                v.Weights = glm::vec4(d[0], d[1], d[2], d[3]);
            }
            // 가끔 Normalized Unsigned Byte/Short로 오는 경우도 있음 (드물지만)
            // 필요하면 추가 처리
        }
    }

    return data;
}


