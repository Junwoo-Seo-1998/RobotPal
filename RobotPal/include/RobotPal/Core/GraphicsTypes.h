#ifndef __GRAPHICSTYPES_H__
#define __GRAPHICSTYPES_H__
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "RobotPal/VertexArray.h"
#include "RobotPal/Shader.h"

struct Mesh {
    std::shared_ptr<VertexArray> vertexArray;
    std::string name;
    uint32_t vertexCount = 0; // DrawArrays
};

struct Material {
    std::shared_ptr<Shader> shader;
    //std::shared_ptr<Texture> mainTexture;
    glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    
    // void Bind() const {
    //     if(shader) {
    //         shader->Bind();
    //         shader->SetFloat4("u_Color", baseColor);
    //         if (mainTexture) {
    //             // shader->SetInt("u_MainTex", 0); 
    //             // mainTexture->Bind(0);
    //         }
    //     }
    // }
};

struct MeshFilter {
    std::shared_ptr<Mesh> mesh; 
};

struct MeshRenderer {
    std::shared_ptr<Material> material;
    // bool castShadows = true;
};

#endif