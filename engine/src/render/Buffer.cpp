#include "limbo/render/Buffer.hpp"
#include "limbo/core/Assert.hpp"

#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace limbo {

// ============================================================================
// VertexBuffer
// ============================================================================

VertexBuffer::~VertexBuffer() {
    destroy();
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_bufferId(other.m_bufferId), m_layout(std::move(other.m_layout)) {
    other.m_bufferId = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_bufferId = other.m_bufferId;
        m_layout = std::move(other.m_layout);
        other.m_bufferId = 0;
    }
    return *this;
}

void VertexBuffer::create(const void* data, u32 size) {
    destroy();

    glCreateBuffers(1, &m_bufferId);
    glNamedBufferData(m_bufferId, size, data, GL_STATIC_DRAW);

    spdlog::debug("VertexBuffer created (ID: {}, size: {} bytes)", m_bufferId, size);
}

void VertexBuffer::create(u32 size) {
    destroy();

    glCreateBuffers(1, &m_bufferId);
    glNamedBufferData(m_bufferId, size, nullptr, GL_DYNAMIC_DRAW);

    spdlog::debug("VertexBuffer created (ID: {}, size: {} bytes, dynamic)", m_bufferId, size);
}

void VertexBuffer::setData(const void* data, u32 size) {
    LIMBO_ASSERT(m_bufferId != 0, "Attempting to set data on invalid vertex buffer");
    glNamedBufferSubData(m_bufferId, 0, size, data);
}

void VertexBuffer::bind() const {
    LIMBO_ASSERT(m_bufferId != 0, "Attempting to bind invalid vertex buffer");
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
}

void VertexBuffer::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::destroy() {
    if (m_bufferId != 0) {
        glDeleteBuffers(1, &m_bufferId);
        spdlog::debug("VertexBuffer destroyed (ID: {})", m_bufferId);
        m_bufferId = 0;
    }
}

// ============================================================================
// IndexBuffer
// ============================================================================

IndexBuffer::~IndexBuffer() {
    destroy();
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_bufferId(other.m_bufferId), m_count(other.m_count) {
    other.m_bufferId = 0;
    other.m_count = 0;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_bufferId = other.m_bufferId;
        m_count = other.m_count;
        other.m_bufferId = 0;
        other.m_count = 0;
    }
    return *this;
}

void IndexBuffer::create(const u32* indices, u32 count) {
    destroy();

    m_count = count;
    glCreateBuffers(1, &m_bufferId);
    glNamedBufferData(m_bufferId, count * sizeof(u32), indices, GL_STATIC_DRAW);

    spdlog::debug("IndexBuffer created (ID: {}, count: {})", m_bufferId, count);
}

void IndexBuffer::bind() const {
    LIMBO_ASSERT(m_bufferId != 0, "Attempting to bind invalid index buffer");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferId);
}

void IndexBuffer::unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::destroy() {
    if (m_bufferId != 0) {
        glDeleteBuffers(1, &m_bufferId);
        spdlog::debug("IndexBuffer destroyed (ID: {})", m_bufferId);
        m_bufferId = 0;
    }
    m_count = 0;
}

// ============================================================================
// VertexArray
// ============================================================================

namespace {
GLenum shaderDataTypeToOpenGLType(ShaderDataType type) {
    switch (type) {
    case ShaderDataType::Float:
    case ShaderDataType::Float2:
    case ShaderDataType::Float3:
    case ShaderDataType::Float4:
    case ShaderDataType::Mat3:
    case ShaderDataType::Mat4:
        return GL_FLOAT;
    case ShaderDataType::Int:
    case ShaderDataType::Int2:
    case ShaderDataType::Int3:
    case ShaderDataType::Int4:
        return GL_INT;
    case ShaderDataType::Bool:
        return GL_BOOL;
    case ShaderDataType::None:
        break;
    }
    LIMBO_ASSERT(false, "Unknown ShaderDataType");
    return 0;
}
}  // namespace

VertexArray::~VertexArray() {
    destroy();
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_arrayId(other.m_arrayId), m_vertexBufferIndex(other.m_vertexBufferIndex),
      m_vertexBuffers(std::move(other.m_vertexBuffers)),
      m_indexBuffer(std::move(other.m_indexBuffer)) {
    other.m_arrayId = 0;
    other.m_vertexBufferIndex = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        destroy();
        m_arrayId = other.m_arrayId;
        m_vertexBufferIndex = other.m_vertexBufferIndex;
        m_vertexBuffers = std::move(other.m_vertexBuffers);
        m_indexBuffer = std::move(other.m_indexBuffer);
        other.m_arrayId = 0;
        other.m_vertexBufferIndex = 0;
    }
    return *this;
}

void VertexArray::create() {
    destroy();
    glCreateVertexArrays(1, &m_arrayId);
    spdlog::debug("VertexArray created (ID: {})", m_arrayId);
}

void VertexArray::bind() const {
    LIMBO_ASSERT(m_arrayId != 0, "Attempting to bind invalid vertex array");
    glBindVertexArray(m_arrayId);
}

void VertexArray::unbind() {
    glBindVertexArray(0);
}

void VertexArray::addVertexBuffer(VertexBuffer&& buffer) {
    LIMBO_ASSERT(m_arrayId != 0, "Vertex array not created");
    LIMBO_ASSERT(!buffer.getLayout().getElements().empty(), "Vertex buffer has no layout");

    bind();
    buffer.bind();

    const auto& layout = buffer.getLayout();
    for (const auto& element : layout) {
        switch (element.type) {
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4: {
            glEnableVertexAttribArray(m_vertexBufferIndex);
            glVertexAttribPointer(
                m_vertexBufferIndex, static_cast<GLint>(element.getComponentCount()),
                shaderDataTypeToOpenGLType(element.type), element.normalized ? GL_TRUE : GL_FALSE,
                static_cast<GLsizei>(layout.getStride()),
                reinterpret_cast<const void*>(static_cast<uintptr_t>(element.offset)));
            m_vertexBufferIndex++;
            break;
        }
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:
        case ShaderDataType::Bool: {
            glEnableVertexAttribArray(m_vertexBufferIndex);
            glVertexAttribIPointer(
                m_vertexBufferIndex, static_cast<GLint>(element.getComponentCount()),
                shaderDataTypeToOpenGLType(element.type), static_cast<GLsizei>(layout.getStride()),
                reinterpret_cast<const void*>(static_cast<uintptr_t>(element.offset)));
            m_vertexBufferIndex++;
            break;
        }
        case ShaderDataType::Mat3:
        case ShaderDataType::Mat4: {
            u32 count = element.getComponentCount();
            u32 colCount = (element.type == ShaderDataType::Mat3) ? 3 : 4;
            for (u32 i = 0; i < colCount; i++) {
                glEnableVertexAttribArray(m_vertexBufferIndex);
                glVertexAttribPointer(m_vertexBufferIndex, static_cast<GLint>(count / colCount),
                                      shaderDataTypeToOpenGLType(element.type),
                                      element.normalized ? GL_TRUE : GL_FALSE,
                                      static_cast<GLsizei>(layout.getStride()),
                                      reinterpret_cast<const void*>(
                                          element.offset + sizeof(f32) * count / colCount * i));
                glVertexAttribDivisor(m_vertexBufferIndex, 1);
                m_vertexBufferIndex++;
            }
            break;
        }
        case ShaderDataType::None:
            LIMBO_ASSERT(false, "Unknown ShaderDataType");
            break;
        }
    }

    m_vertexBuffers.push_back(std::move(buffer));
}

void VertexArray::setIndexBuffer(IndexBuffer&& buffer) {
    LIMBO_ASSERT(m_arrayId != 0, "Vertex array not created");

    bind();
    buffer.bind();

    m_indexBuffer = std::move(buffer);
}

void VertexArray::destroy() {
    if (m_arrayId != 0) {
        glDeleteVertexArrays(1, &m_arrayId);
        spdlog::debug("VertexArray destroyed (ID: {})", m_arrayId);
        m_arrayId = 0;
    }
    m_vertexBufferIndex = 0;
    m_vertexBuffers.clear();
    m_indexBuffer = IndexBuffer{};
}

}  // namespace limbo
