#ifndef __IBL_BAKER_H__
#define __IBL_BAKER_H__
#include <glm/glm.hpp>
#include "RobotPal/Core/ResourceID.h"
#include "RobotPal/GlobalComponents.h"
#include <memory>
#include <vector>

class VertexArray;
class Texture;

class IBLBaker {
public:
    static void Init();
    static GlobalLighting Bake(ResourceID hdrTextureID);

private:
    static void RenderCube();
    static void RenderQuad();
    static void ComputeSH(std::shared_ptr<Texture> cubemap, glm::vec3* shCoeffs);

    static std::shared_ptr<VertexArray> s_CubeVAO;
    static std::shared_ptr<VertexArray> s_QuadVAO;
};

#endif