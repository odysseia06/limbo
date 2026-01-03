#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <vector>
#include <initializer_list>

namespace limbo {

// Shader data types
enum class ShaderDataType : u8 {
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    Mat3,
    Mat4,
    Bool
};

// Get size in bytes for a shader data type
constexpr u32 shaderDataTypeSize(ShaderDataType type) {
    switch (type) {
    case ShaderDataType::Float:
        return 4;
    case ShaderDataType::Float2:
        return 4 * 2;
    case ShaderDataType::Float3:
        return 4 * 3;
    case ShaderDataType::Float4:
        return 4 * 4;
    case ShaderDataType::Int:
        return 4;
    case ShaderDataType::Int2:
        return 4 * 2;
    case ShaderDataType::Int3:
        return 4 * 3;
    case ShaderDataType::Int4:
        return 4 * 4;
    case ShaderDataType::Mat3:
        return 4 * 3 * 3;
    case ShaderDataType::Mat4:
        return 4 * 4 * 4;
    case ShaderDataType::Bool:
        return 1;
    case ShaderDataType::None:
        return 0;
    }
    return 0;
}

// Get component count for a shader data type
constexpr u32 shaderDataTypeComponentCount(ShaderDataType type) {
    switch (type) {
    case ShaderDataType::Float:
        return 1;
    case ShaderDataType::Float2:
        return 2;
    case ShaderDataType::Float3:
        return 3;
    case ShaderDataType::Float4:
        return 4;
    case ShaderDataType::Int:
        return 1;
    case ShaderDataType::Int2:
        return 2;
    case ShaderDataType::Int3:
        return 3;
    case ShaderDataType::Int4:
        return 4;
    case ShaderDataType::Mat3:
        return 3 * 3;
    case ShaderDataType::Mat4:
        return 4 * 4;
    case ShaderDataType::Bool:
        return 1;
    case ShaderDataType::None:
        return 0;
    }
    return 0;
}

// Single element in a buffer layout
struct BufferElement {
    String name;
    ShaderDataType type;
    u32 size;
    u32 offset;
    bool normalized;

    BufferElement() = default;

    BufferElement(ShaderDataType dataType, String elementName, bool isNormalized = false)
        : name(std::move(elementName)), type(dataType), size(shaderDataTypeSize(dataType)),
          offset(0), normalized(isNormalized) {}

    [[nodiscard]] u32 getComponentCount() const { return shaderDataTypeComponentCount(type); }
};

// Layout of vertex buffer data
class LIMBO_API BufferLayout {
public:
    BufferLayout() = default;

    BufferLayout(std::initializer_list<BufferElement> elements) : m_elements(elements) {
        calculateOffsetsAndStride();
    }

    [[nodiscard]] const std::vector<BufferElement>& getElements() const { return m_elements; }
    [[nodiscard]] u32 getStride() const { return m_stride; }

    [[nodiscard]] auto begin() const { return m_elements.begin(); }
    [[nodiscard]] auto end() const { return m_elements.end(); }

private:
    void calculateOffsetsAndStride() {
        u32 offset = 0;
        m_stride = 0;
        for (auto& element : m_elements) {
            element.offset = offset;
            offset += element.size;
            m_stride += element.size;
        }
    }

    std::vector<BufferElement> m_elements;
    u32 m_stride = 0;
};

// Vertex buffer (VBO)
class LIMBO_API VertexBuffer {
public:
    VertexBuffer() = default;
    ~VertexBuffer();

    // Non-copyable
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // Movable
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    // Create with initial data
    void create(const void* data, u32 size);

    // Create empty buffer with given size
    void create(u32 size);

    // Update buffer data
    void setData(const void* data, u32 size);

    void bind() const;
    static void unbind();

    [[nodiscard]] bool isValid() const { return m_bufferId != 0; }

    void setLayout(const BufferLayout& layout) { m_layout = layout; }
    [[nodiscard]] const BufferLayout& getLayout() const { return m_layout; }

    [[nodiscard]] u32 getNativeHandle() const { return m_bufferId; }

private:
    void destroy();

    u32 m_bufferId = 0;
    BufferLayout m_layout;
};

// Index buffer (EBO/IBO)
class LIMBO_API IndexBuffer {
public:
    IndexBuffer() = default;
    ~IndexBuffer();

    // Non-copyable
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    // Movable
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    // Create with data
    void create(const u32* indices, u32 count);

    void bind() const;
    static void unbind();

    [[nodiscard]] bool isValid() const { return m_bufferId != 0; }
    [[nodiscard]] u32 getCount() const { return m_count; }

    [[nodiscard]] u32 getNativeHandle() const { return m_bufferId; }

private:
    void destroy();

    u32 m_bufferId = 0;
    u32 m_count = 0;
};

// Vertex array object (VAO)
class LIMBO_API VertexArray {
public:
    VertexArray() = default;
    ~VertexArray();

    // Non-copyable
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    // Movable
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void create();

    void bind() const;
    static void unbind();

    void addVertexBuffer(VertexBuffer&& buffer);
    void setIndexBuffer(IndexBuffer&& buffer);

    [[nodiscard]] bool isValid() const { return m_arrayId != 0; }
    [[nodiscard]] const std::vector<VertexBuffer>& getVertexBuffers() const {
        return m_vertexBuffers;
    }
    [[nodiscard]] const IndexBuffer& getIndexBuffer() const { return m_indexBuffer; }
    [[nodiscard]] bool hasIndexBuffer() const { return m_indexBuffer.isValid(); }

    [[nodiscard]] u32 getNativeHandle() const { return m_arrayId; }

private:
    void destroy();

    u32 m_arrayId = 0;
    u32 m_vertexBufferIndex = 0;
    std::vector<VertexBuffer> m_vertexBuffers;
    IndexBuffer m_indexBuffer;
};

}  // namespace limbo
