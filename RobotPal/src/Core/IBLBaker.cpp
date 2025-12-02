#include "RobotPal/Core/IBLBaker.h"
#include "RobotPal/Core/AssetManager.h"
#include "RobotPal/Core/Texture.h"
#include "RobotPal/Shader.h"
#include "RobotPal/Core/Framebuffer.h"
#include "RobotPal/VertexArray.h"
#include <glad/gles2.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

std::shared_ptr<VertexArray> IBLBaker::s_CubeVAO = nullptr;
std::shared_ptr<VertexArray> IBLBaker::s_QuadVAO = nullptr;

void IBLBaker::Init() {
    if (s_CubeVAO) return;
    
    // Cube VAO (Positions only)
    float cubeVertices[] = {
        // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left  
    };
    s_CubeVAO = VertexArray::Create();
    auto vb = std::make_shared<VertexBuffer>(cubeVertices, sizeof(cubeVertices));
    vb->SetLayout({ {DataType::Float3, "a_Position"}, {DataType::Float3, "a_Normal"}, {DataType::Float2, "a_Tex"} });
    s_CubeVAO->AddVertexBuffer(vb);

    // Quad VAO
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    s_QuadVAO = VertexArray::Create();
    auto qvb = std::make_shared<VertexBuffer>(quadVertices, sizeof(quadVertices));
    qvb->SetLayout({ {DataType::Float3, "a_Position"}, {DataType::Float2, "a_TexCoord"} });
    s_QuadVAO->AddVertexBuffer(qvb);
}

GlobalLighting IBLBaker::Bake(ResourceID hdrTextureID) {
    Init();
    GlobalLighting result;
    
    // 1. Setup Framebuffer & Matrices
    auto captureFBO = Framebuffer::Create(1024, 1024, TextureFormat::RGB8, true);
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // 2. EquiToCube
    int envSize = 1024;
    auto envCubemap = std::make_shared<Texture>(envSize, envSize, TextureFormat::RGB16F, TextureType::TextureCube);
    auto equiShader = AssetManager::Get().GetShader("./Assets/Shaders/EquiToCube.glsl");
    //auto equiShader=Shader::CreateFromSource("EquiToCube", "./Assets/Shaders/EquiToCube.vert", "./Assets/Shaders/EquiToCube.frag");
    auto hdrTex = AssetManager::Get().GetTextureHDR(hdrTextureID);

    equiShader->Bind();
    equiShader->SetInt("equirectangularMap", 0);
    equiShader->SetMat4("projection", captureProjection);
    hdrTex->Bind(0);
    //glDisable(GL_CULL_FACE);
    glViewport(0, 0, envSize, envSize);
    captureFBO->Resize(envSize, envSize);
    for(int i=0; i<6; ++i) {
        equiShader->SetMat4("view", captureViews[i]);
        captureFBO->BindTextureFace(envCubemap, i, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap->GetID());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    //glEnable(GL_CULL_FACE);
    // // 3. Diffuse SH
    // ComputeSH(envCubemap, result.shCoeffs);

    // // 4. Prefilter Specular
    // int prefilterSize = 128;
    // auto prefilterMap = std::make_shared<Texture>(prefilterSize, prefilterSize, TextureFormat::RGB16F, TextureType::TextureCube);
    // glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap->GetID());
    // glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    // auto prefilterShader = AssetManager::Get().GetShader("./Assets/Shaders/Prefilter.glsl");
    // //auto prefilterShader=Shader::CreateFromSource("EquiToCube", "./Assets/Shaders/Prefilter.vert", "./Assets/Shaders/Prefilter.frag");
    // prefilterShader->Bind();
    // prefilterShader->SetInt("environmentMap", 0);
    // prefilterShader->SetMat4("projection", captureProjection);
    // envCubemap->Bind(0);

    // int maxMipLevels = 5;
    // for(int mip=0; mip<maxMipLevels; ++mip) {
    //     int mipWidth  = prefilterSize * std::pow(0.5, mip);
    //     int mipHeight = prefilterSize * std::pow(0.5, mip);
    //     captureFBO->Resize(mipWidth, mipHeight);
        
    //     float roughness = (float)mip / (float)(maxMipLevels - 1);
    //     prefilterShader->SetFloat("roughness", roughness);
        
    //     for(int i=0; i<6; ++i) {
    //         prefilterShader->SetMat4("view", captureViews[i]);
    //         captureFBO->BindTextureFace(prefilterMap, i, mip);
    //         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //         RenderCube();
    //     }
    // }

    // // 5. BRDF LUT
    // auto brdfLUT = std::make_shared<Texture>(512, 512, TextureFormat::RGB16F); // 2D
    // auto brdfShader = AssetManager::Get().GetShader("./Assets/Shaders/BRDF.glsl");
    // //auto brdfShader=Shader::CreateFromSource("EquiToCube", "./Assets/Shaders/BRDF.vert", "./Assets/Shaders/BRDF.frag");
    // brdfShader->Bind();
    
    // captureFBO->Resize(512, 512);
    // captureFBO->BindTextureFace(brdfLUT, 0, 0);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // RenderQuad();
    
    // captureFBO->Unbind();

    // // 6. Register Resources
    // result.prefilteredMap = AssetManager::Get().AddRuntimeTextureHDR(prefilterMap, "IBL_Prefilter");
    // result.brdfLUT = AssetManager::Get().AddRuntimeTextureHDR(brdfLUT, "IBL_BRDF_LUT");

    ResourceID envID = AssetManager::Get().AddRuntimeTextureHDR(envCubemap, "Generated/IBL_Environment");
    result.environmentMap = envID;
    return result;
}

void IBLBaker::ComputeSH(std::shared_ptr<Texture> cubemap, glm::vec3* shCoeffs) {
    for(int i=0; i<9; ++i) shCoeffs[i] = glm::vec3(0.0f);
    
    int mipLevel = 3; // 64x64 정도
    int size = 64;
    std::vector<float> buffer(size * size * 3);
    auto readFBO = Framebuffer::Create(size, size, TextureFormat::RGB8, false);

    // 축 정의
    glm::vec3 dirs[] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
    glm::vec3 ups[]  = { {0,-1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}, {0,-1,0}, {0,-1,0} };
    glm::vec3 rights[]={ {0,0,-1}, {0,0,1}, {1,0,0}, {1,0,0}, {1,0,0}, {-1,0,0} };

    float weightSum = 0.0f;
    for(int face=0; face<6; ++face) {
        readFBO->BindTextureFace(cubemap, face, mipLevel);
        glReadPixels(0, 0, size, size, GL_RGB, GL_FLOAT, buffer.data());
        
        for(int y=0; y<size; ++y) {
            for(int x=0; x<size; ++x) {
                float u = ((x+0.5f)/size)*2.0f - 1.0f;
                float v = ((y+0.5f)/size)*2.0f - 1.0f;
                glm::vec3 dir = glm::normalize(dirs[face] + rights[face]*u + ups[face]*v);
                
                int idx = (y*size + x)*3;
                glm::vec3 color(buffer[idx], buffer[idx+1], buffer[idx+2]);
                
                float temp = 1.0f + u*u + v*v;
                float weight = 4.0f / (sqrt(temp) * temp);

                float sh[9];
                sh[0] = 0.282095f;
                sh[1] = 0.488603f * dir.y; sh[2] = 0.488603f * dir.z; sh[3] = 0.488603f * dir.x;
                sh[4] = 1.092548f * dir.x * dir.y; sh[5] = 1.092548f * dir.y * dir.z;
                sh[6] = 0.315392f * (3.0f*dir.z*dir.z - 1.0f); sh[7] = 1.092548f * dir.x * dir.z;
                sh[8] = 0.546274f * (dir.x*dir.x - dir.y*dir.y);

                for(int i=0; i<9; ++i) shCoeffs[i] += color * sh[i] * weight;
                weightSum += weight;
            }
        }
    }
    
    float norm = (4.0f * 3.14159f) / weightSum;
    for(int i=0; i<9; ++i) shCoeffs[i] *= norm;
}

void IBLBaker::RenderCube() {
    s_CubeVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    s_CubeVAO->UnBind();
}
void IBLBaker::RenderQuad() {
    s_QuadVAO->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    s_QuadVAO->UnBind();
}