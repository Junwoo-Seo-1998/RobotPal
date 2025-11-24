#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <memory>
#include <string>
#include <vector>

enum class DataType
{
	None = 0, Bool, Int, Int2, Int3, Int4, Float, Float2, Float3, Float4, Mat3, Mat4,
};

static uint32_t DataTypeSize(DataType type)
{
	switch (type)
	{
		case DataType::Bool:	return 1;
		case DataType::Int:		return 4;
		case DataType::Int2:	return 4 * 2;
		case DataType::Int3:	return 4 * 3;
		case DataType::Int4:	return 4 * 4;
		case DataType::Float:	return 4;
		case DataType::Float2:	return 4 * 2;
		case DataType::Float3:	return 4 * 3;
		case DataType::Float4:	return 4 * 4;
		case DataType::Mat3:	return 4 * 3 * 3;
		case DataType::Mat4:	return 4 * 4 * 4;
	}
	return 0;
}

struct BufferElement 
{
	std::string Name;
	DataType Type;
	uint32_t Size;
	size_t Offset;
	bool Normalized;

	BufferElement(DataType type, const std::string& name, bool normalized = false)
		: Name(name), Type(type), Size(DataTypeSize(type)), Offset(0), Normalized(normalized)
	{
	}

	uint32_t GetComponentCount() const
	{
		switch (Type)
		{
			case DataType::Bool:	return 1;
			case DataType::Int:		return 1;
			case DataType::Int2:	return 2;
			case DataType::Int3:	return 3;
			case DataType::Int4:	return 4;
			case DataType::Float:	return 1;
			case DataType::Float2:	return 2;
			case DataType::Float3:	return 3;
			case DataType::Float4:	return 4;
			case DataType::Mat3:	return 3; // 3* float3
			case DataType::Mat4:	return 4; // 4* float4
		}
		return 0;
	}
};

class BufferLayout 
{
public:
	BufferLayout() {}
	BufferLayout(const std::initializer_list<BufferElement>& elements)
		: m_Elements(elements)
	{
		CalculateOffsetsAndStride();
	}

	uint32_t GetStride() const { return m_Stride; }
	const std::vector<BufferElement>& GetElements() const { return m_Elements; }

	std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
	std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
	std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
	std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
private:
	void CalculateOffsetsAndStride()
	{
		size_t offset = 0;
		m_Stride = 0;
		for (auto& element : m_Elements)
		{
			element.Offset = offset;
			offset += element.Size;
			m_Stride += element.Size;
		}
	}
private:
	std::vector<BufferElement> m_Elements;
	uint32_t m_Stride = 0;
};


class VertexBuffer 
{
public:
    static std::shared_ptr<VertexBuffer> Create(float* vertices, uint32_t byteSize);
    static std::shared_ptr<VertexBuffer> Create(uint32_t byteSize);
    VertexBuffer(float* vertices, uint32_t byteSize);
    VertexBuffer(uint32_t byteSize);
    ~VertexBuffer();

    void SetData(const void* data, uint32_t byteSize);

    void Bind() const;
    void UnBind() const;

    const BufferLayout& GetLayout() const;
    void SetLayout(const BufferLayout& layout);
private:
    unsigned int m_Buffer;
    BufferLayout m_Layout;
};

class IndexBuffer
{
public:
    static std::shared_ptr<IndexBuffer> Create(uint32_t* indices, uint32_t count);
    IndexBuffer(uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    void Bind() const;
    void UnBind() const;

    uint32_t GetCount() const;
private:
    uint32_t m_Buffer;
    uint32_t m_Count;
};

#endif