#ifndef __RENDERCOMMAND_H__
#define __RENDERCOMMAND_H__
#include <glm/glm.hpp>
#include <memory>
#include "RobotPal/VertexArray.h" // 기존 클래스 활용

class RenderCommand {
public:
    static void Init();

    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    static void SetClearColor(const glm::vec4& color);

    static void Clear();

    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray);
    
    static void DrawArrays(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);
};
#endif