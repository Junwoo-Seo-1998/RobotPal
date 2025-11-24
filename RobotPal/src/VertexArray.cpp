#include "RobotPal/VertexArray.h"
#include <glad/gles2.h>

static GLenum ShaderDataTypeToOpenGLBaseType(DataType type)
{
	switch (type)
	{
		case DataType::Bool:	return GL_BOOL;
		case DataType::Int:		return GL_INT;
		case DataType::Int2:	return GL_INT;
		case DataType::Int3:	return GL_INT;
		case DataType::Int4:	return GL_INT;
		case DataType::Float:	return GL_FLOAT;
		case DataType::Float2:	return GL_FLOAT;
		case DataType::Float3:	return GL_FLOAT;
		case DataType::Float4:	return GL_FLOAT;
		case DataType::Mat3:	return GL_FLOAT;
		case DataType::Mat4:	return GL_FLOAT;
	}
	return 0;
}

std::shared_ptr<VertexArray> VertexArray::Create()
{
    return std::make_shared<VertexArray>();
}

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_RendererID);
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_RendererID);
}

void VertexArray::Bind() const
{
    glBindVertexArray(m_RendererID);
}

void VertexArray::UnBind() const
{
    glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
{
    glBindVertexArray(m_RendererID);
    vertexBuffer->Bind();

    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout)
    {
        glEnableVertexAttribArray(m_VertexBufferIndexOffset);
        glVertexAttribPointer(m_VertexBufferIndexOffset,
            element.GetComponentCount(),
            ShaderDataTypeToOpenGLBaseType(element.Type),
            element.Normalized ? GL_TRUE : GL_FALSE,
            layout.GetStride(),
            (const void*)(uintptr_t)element.Offset);
        m_VertexBufferIndexOffset++;
    }
    m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
    glBindVertexArray(m_RendererID);
    indexBuffer->Bind();
    m_IndexBuffer = indexBuffer;
}

const std::vector<std::shared_ptr<VertexBuffer>>& VertexArray::GetVertexBuffers() const
{
    return m_VertexBuffers;
}

const std::shared_ptr<IndexBuffer>& VertexArray::GetIndexBuffer() const
{
    return m_IndexBuffer;
}