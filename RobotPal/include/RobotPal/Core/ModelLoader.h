#ifndef __MODELLOADER_H__
#define __MODELLOADER_H__
#include <string>
#include <flecs.h>
#include "tiny_gltf.h"
#include "RobotPal/Scene.h"
#include "RobotPal/Core/GraphicsTypes.h"

class ModelLoader {
public:
    static bool LoadModelData(const std::string& path, ModelResource& outResource);

private:
    static int ProcessNodeRecursive(tinygltf::Model& model, const tinygltf::Node& gltfNode, int parentIdx, ModelResource& outResource);

    static PrimitiveData ExtractPrimitiveData(tinygltf::Model &model, const tinygltf::Primitive &primitive);
    static MeshData ProcessMesh(tinygltf::Model& model, const tinygltf::Mesh& gltfMesh);
};


//todo: PrefabFactory.h
class PrefabFactory {
public:
    // ModelResource -> ECS Prefab Entity 변환
    static flecs::entity CreatePrefab(flecs::world& ecs, const std::string& name, const ModelResource& res);

private:
    static void CreateNodesRecursive(flecs::world& ecs, flecs::entity parentPrefab, const ModelResource& res, int nodeIdx);
};

#endif