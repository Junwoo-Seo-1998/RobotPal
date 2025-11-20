#ifndef __MODELLOADER_H__
#define __MODELLOADER_H__
#include <string>
#include <flecs.h>
#include "tiny_gltf.h"
#include "RobotPal/Scene.h"
#include "RobotPal/Core/GraphicsTypes.h"

class ModelLoader {
public:
    static flecs::entity LoadModel(Scene* scene, const std::string& filepath);

private:
    static void ProcessNode(Scene* scene, flecs::entity parent, tinygltf::Model& model, tinygltf::Node& node);
    static std::shared_ptr<Mesh> ProcessMesh(tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& name);
};

#endif