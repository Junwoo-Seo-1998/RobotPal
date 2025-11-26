#ifndef __COMPONENTS_H__
#define __COMPONENTS_H__
#include "RobotPal/Core/ResourceID.h"
#include "RobotPal/Core/GraphicsTypes.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>

struct Position : public glm::vec3
{
    using glm::vec3::vec3;

    Position() : glm::vec3(0.0f) {}
    Position(const glm::vec3 &v) : glm::vec3(v) {}
};

struct Rotation : public glm::vec3
{
    using glm::vec3::vec3;

    Rotation() : glm::vec3(0.0f) {}
    Rotation(const glm::vec3 &v) : glm::vec3(v) {}
};

struct Scale : public glm::vec3
{
    using glm::vec3::vec3;

    Scale() : glm::vec3(1.0f) {}
    Scale(float s) : glm::vec3(s, s, s) {}
    Scale(const glm::vec3 &v) : glm::vec3(v) {}
};

struct TransformMatrix : public glm::mat4
{
    using glm::mat4::mat4;

    TransformMatrix() : glm::mat4(1.0f) {}
    TransformMatrix(const glm::mat4 &m) : glm::mat4(m) {}
};

struct Local
{};
struct World
{};


struct MeshFilter 
{
    ResourceID meshID;
};

struct MeshRenderer {
    std::shared_ptr<Material> material;
    // bool castShadows = true;
};

#endif