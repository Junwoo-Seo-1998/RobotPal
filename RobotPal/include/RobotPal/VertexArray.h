#ifndef __VERTEX_ARRAY_H__
#define __VERTEX_ARRAY_H__

#include "RobotPal/Buffer.h"
#include <memory>
#include <vector>

class VertexArray
{
public:
    static std::shared_ptr<VertexArray> Create();
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void UnBind() const;

    void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
    void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

    const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const;
    const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const;

private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferIndexOffset = 0;
    std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

#endif