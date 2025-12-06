#include "RobotPal/Buffer.h"
#include <glad/gles2.h>

std::shared_ptr<VertexBuffer> VertexBuffer::Create(void *vertices, uint32_t byteSize)
{
    return std::make_shared<VertexBuffer>(vertices, byteSize);
}

std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t byteSize)
{
    return std::make_shared<VertexBuffer>(byteSize);
}

VertexBuffer::VertexBuffer(void *vertices, uint32_t byteSize)
{
    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ARRAY_BUFFER, byteSize, vertices, GL_STATIC_DRAW);
}

VertexBuffer::VertexBuffer(uint32_t byteSize)
{
    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ARRAY_BUFFER, byteSize, nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers(1, &m_Buffer);
}

void VertexBuffer::SetData(const void *data, uint32_t byteSize)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, byteSize, data);
}

void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
}

void VertexBuffer::UnBind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

const BufferLayout &VertexBuffer::GetLayout() const
{
    return m_Layout;
}

void VertexBuffer::SetLayout(const BufferLayout &layout)
{
    m_Layout = layout;
}

std::shared_ptr<IndexBuffer> IndexBuffer::Create(void *indices, uint32_t count)
{
    return std::make_shared<IndexBuffer>(indices, count);
}

IndexBuffer::IndexBuffer(void *indices, uint32_t count)
    : m_Count(count)
{
    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers(1, &m_Buffer);
}

void IndexBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffer);
}

void IndexBuffer::UnBind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

uint32_t IndexBuffer::GetCount() const
{
    return m_Count;
}
