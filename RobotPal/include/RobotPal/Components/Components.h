#ifndef __COMPONENTS_H__
#define __COMPONENTS_H__
#include "RobotPal/Core/ResourceID.h"
#include "RobotPal/Core/GraphicsTypes.h"
#include "RobotPal/Core/Framebuffer.h"
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
    std::vector<ResourceID> materials; 
    
    bool castShadows = true;
    bool receiveShadows = true;
};

//-------------카메라 관련--------------

struct Camera {
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    bool useFisheye=false;
};

// // 렌더링 순서 제어용 TODO
// struct RenderPriority {
//     int value = 0; 
// };

// 어떤 FBO에 그릴지 (관계로 사용해도 되고 컴포넌트로 써도 됨)
struct RenderTarget {
    std::shared_ptr<Framebuffer> fbo; 
};

// // 무엇을 볼 것인가 (비트마스크) TODO
// struct LayerMask {
//     uint32_t layers = 0xFFFFFFFF; // 기본적으로 모든 레이어 보기
// };

//------------네트워크 관련---------------

struct VideoSender {
    int width, height;
    float fpsLimit;
    float timeSinceLastFrame;
};

#endif