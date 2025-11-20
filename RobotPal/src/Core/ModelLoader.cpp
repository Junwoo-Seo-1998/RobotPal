#include "RobotPal/Core/ModelLoader.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Components/Components.h"
#include "RobotPal/Core/GraphicsTypes.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
flecs::entity ModelLoader::LoadModel(Scene *scene, const std::string &filepath)
{
    if (AssetManager::Get().HasPrefab(filepath)) {
        return AssetManager::Get().GetPrefab(filepath);
    }

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
        ProcessNode(scene, rootHandle, model, model.nodes[nodeIdx]);
    }

    AssetManager::Get().AddPrefab(filepath, rootHandle);

    return rootHandle;
}

void ModelLoader::ProcessNode(Scene *scene, flecs::entity parent, tinygltf::Model &model, tinygltf::Node &node)
{
    Entity entity = scene->CreateEntity(node.name.empty() ? "Node" : node.name);
    flecs::entity handle = entity.GetHandle();

    handle.child_of(parent);

    entity.AddComponent<TransformComponent>();
    auto* transform = entity.GetComponent<TransformComponent>();
    if (!node.translation.empty()) {
        transform->Position = glm::make_vec3(node.translation.data());
    }
    if (!node.rotation.empty()) {
        glm::quat q = glm::make_quat(node.rotation.data());
        transform->Rotation = glm::degrees(glm::eulerAngles(q));
    }
    if (!node.scale.empty()) {
        transform->Scale = glm::make_vec3(node.scale.data());
    }
    if (!node.matrix.empty()) {
        transform->WorldMatrix = glm::make_mat4(node.matrix.data());
        // TODO: Decompose matrix to get T, R, S
    }

    if (node.mesh > -1) {
        const auto& gltfMesh = model.meshes[node.mesh];
        for (const auto& primitive : gltfMesh.primitives) {
            // In a real engine, a single node might have multiple primitives,
            // creating multiple renderable sub-entities. For this project,
            // we'll simplify and assume one primitive per node, or just take the first.
            // A more robust approach would create a child entity for each primitive.

            auto mesh = ProcessMesh(model, primitive, gltfMesh.name);
            entity.SetComponent<MeshFilter>({mesh});

            // Create a default material.
            // In a full engine, you'd process material properties from the glTF file.
            auto material = std::make_shared<Material>();
            material->shader = AssetManager::Get().GetShader("default"); // Assuming a "default" shader exists
            entity.SetComponent<MeshRenderer>({material});
            
            // For simplicity, we only process the first primitive
            break; 
        }
    }

    for (int childIdx : node.children) {
        ProcessNode(scene, handle, model, model.nodes[childIdx]);
    }
}

std::shared_ptr<Mesh> ModelLoader::ProcessMesh(tinygltf::Model &model, const tinygltf::Primitive &primitive, const std::string &name)
{
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    size_t vertexCount = 0;

    // Positions
    const float* positions = nullptr;
    if (primitive.attributes.count("POSITION")) {
        const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        vertexCount = accessor.count;
    }

    // Normals
    const float* normals = nullptr;
    if (primitive.attributes.count("NORMAL")) {
        const auto& accessor = model.accessors[primitive.attributes.at("NORMAL")];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    }

    // TexCoords
    const float* texCoords = nullptr;
    if (primitive.attributes.count("TEXCOORD_0")) {
        const auto& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        texCoords = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    }

    // Interleave vertex data
    vertices.reserve(vertexCount * 8); // 3 pos, 3 norm, 2 uv
    for (size_t i = 0; i < vertexCount; ++i) {
        vertices.push_back(positions[i * 3 + 0]);
        vertices.push_back(positions[i * 3 + 1]);
        vertices.push_back(positions[i * 3 + 2]);

        if (normals) {
            vertices.push_back(normals[i * 3 + 0]);
            vertices.push_back(normals[i * 3 + 1]);
            vertices.push_back(normals[i * 3 + 2]);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }

        if (texCoords) {
            vertices.push_back(texCoords[i * 2 + 0]);
            vertices.push_back(texCoords[i * 2 + 1]);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    // Indices
    const auto& indexAccessor = model.accessors[primitive.indices];
    const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
    const auto& indexBuffer = model.buffers[indexBufferView.buffer];
    const void* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

    indices.reserve(indexAccessor.count);
    switch (indexAccessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                indices.push_back(static_cast<const uint8_t*>(indexData)[i]);
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                indices.push_back(static_cast<const uint16_t*>(indexData)[i]);
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                indices.push_back(static_cast<const uint32_t*>(indexData)[i]);
            }
            break;
        default:
            break;
    }
    
    auto mesh = std::make_shared<Mesh>();
    mesh->name = name;
    
    auto va = VertexArray::Create();
    auto vb = VertexBuffer::Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));
    vb->SetLayout({
        { DataType::Float3, "a_Position" },
        { DataType::Float3, "a_Normal" },
        { DataType::Float2, "a_TexCoord" },
    });
    va->AddVertexBuffer(vb);

    auto ib = IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
    va->SetIndexBuffer(ib);

    mesh->vertexArray = va;
    mesh->vertexCount = static_cast<uint32_t>(indices.size());

    return mesh;
}


