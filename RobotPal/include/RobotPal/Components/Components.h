#ifndef __COMPONENTS_H__
#define __COMPONENTS_H__
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>
struct TransformComponent
{

    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // Euler Angles (Degrees)
    glm::vec3 Scale    = { 1.0f, 1.0f, 1.0f };

    glm::mat4 WorldMatrix = glm::mat4(1.0f);


    glm::mat4 GetLocalMatrix() const {
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), Position);

        mat *= glm::mat4_cast(glm::quat(glm::radians(Rotation)));
        
        mat = glm::scale(mat, Scale);
        return mat;
    }

    glm::vec3 GetForward() const {
        glm::vec3 forward;

        float radX = glm::radians(Rotation.x);
        float radY = glm::radians(Rotation.y);
        
        forward.x = -sin(radY) * cos(radX);
        forward.y = sin(radX);
        forward.z = -cos(radY) * cos(radX);
        return glm::normalize(forward);
    }

    glm::vec3 GetRight() const {
        return glm::normalize(glm::cross(GetForward(), glm::vec3(0, 1, 0)));
    }

    glm::vec3 GetUp() const {
        return glm::normalize(glm::cross(GetRight(), GetForward()));
    }
};

#endif