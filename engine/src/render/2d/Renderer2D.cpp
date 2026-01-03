#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/common/Shader.hpp"
#include "limbo/render/common/Buffer.hpp"
#include "limbo/render/common/Camera.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <memory>

namespace limbo {

// Maximum quads per batch
static constexpr u32 MaxQuads = 10000;
static constexpr u32 MaxVertices = MaxQuads * 4;
static constexpr u32 MaxIndices = MaxQuads * 6;
static constexpr u32 MaxTextureSlots = 32;  // TODO: Query from GPU

struct QuadVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    f32 texIndex;
    f32 tilingFactor;
};

struct Renderer2DData {
    Unique<VertexArray> quadVAO;
    Unique<VertexBuffer> quadVBO;
    Unique<IndexBuffer> quadIBO;
    Unique<Shader> quadShader;
    Unique<Texture2D> whiteTexture;

    u32 quadIndexCount = 0;
    QuadVertex* quadVertexBufferBase = nullptr;
    QuadVertex* quadVertexBufferPtr = nullptr;

    std::array<const Texture2D*, MaxTextureSlots> textureSlots;
    u32 textureSlotIndex = 1;  // 0 = white texture

    glm::vec4 quadVertexPositions[4];

    Renderer2D::Statistics stats;
};

static Renderer2DData s_data;

void Renderer2D::init() {
    // Create VAO
    s_data.quadVAO = make_unique<VertexArray>();
    s_data.quadVAO->create();

    // Create VBO with dynamic storage
    s_data.quadVBO = make_unique<VertexBuffer>();
    s_data.quadVBO->create(nullptr, MaxVertices * sizeof(QuadVertex));
    s_data.quadVBO->setLayout({{ShaderDataType::Float3, "a_Position"},
                               {ShaderDataType::Float4, "a_Color"},
                               {ShaderDataType::Float2, "a_TexCoord"},
                               {ShaderDataType::Float, "a_TexIndex"},
                               {ShaderDataType::Float, "a_TilingFactor"}});
    s_data.quadVAO->addVertexBuffer(std::move(*s_data.quadVBO));

    // Allocate CPU-side vertex buffer
    s_data.quadVertexBufferBase = new QuadVertex[MaxVertices];

    // Create index buffer
    auto* indices = new u32[MaxIndices];
    u32 offset = 0;
    for (u32 i = 0; i < MaxIndices; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;

        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;

        offset += 4;
    }

    s_data.quadIBO = make_unique<IndexBuffer>();
    s_data.quadIBO->create(indices, MaxIndices);
    s_data.quadVAO->setIndexBuffer(std::move(*s_data.quadIBO));
    delete[] indices;

    // Create white texture (1x1 white pixel)
    s_data.whiteTexture = make_unique<Texture2D>();
    u32 whitePixel = 0xFFFFFFFF;
    TextureSpec whiteSpec;
    whiteSpec.width = 1;
    whiteSpec.height = 1;
    whiteSpec.format = TextureFormat::RGBA8;
    (void)s_data.whiteTexture->create(whiteSpec, &whitePixel);

    // Initialize texture slots
    s_data.textureSlots[0] = s_data.whiteTexture.get();

    // Create batch shader
    s_data.quadShader = make_unique<Shader>();

    // Embedded shader source for batched rendering
    const char* vertexSource = R"(
        #version 450 core

        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec4 a_Color;
        layout(location = 2) in vec2 a_TexCoord;
        layout(location = 3) in float a_TexIndex;
        layout(location = 4) in float a_TilingFactor;

        uniform mat4 u_ViewProjection;

        out vec4 v_Color;
        out vec2 v_TexCoord;
        out flat float v_TexIndex;
        out float v_TilingFactor;

        void main() {
            v_Color = a_Color;
            v_TexCoord = a_TexCoord;
            v_TexIndex = a_TexIndex;
            v_TilingFactor = a_TilingFactor;
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 450 core

        in vec4 v_Color;
        in vec2 v_TexCoord;
        in flat float v_TexIndex;
        in float v_TilingFactor;

        uniform sampler2D u_Textures[32];

        out vec4 o_Color;

        void main() {
            int index = int(v_TexIndex);
            vec4 texColor = texture(u_Textures[index], v_TexCoord * v_TilingFactor);
            o_Color = texColor * v_Color;
        }
    )";

    auto result = s_data.quadShader->loadFromSource(vertexSource, fragmentSource);
    if (!result) {
        // Log error but continue - will just render nothing
    }

    // Set up texture sampler array
    s_data.quadShader->bind();
    i32 samplers[MaxTextureSlots];
    for (u32 i = 0; i < MaxTextureSlots; ++i) {
        samplers[i] = static_cast<i32>(i);
    }
    glUniform1iv(glGetUniformLocation(s_data.quadShader->getNativeHandle(), "u_Textures"),
                 static_cast<GLsizei>(MaxTextureSlots), samplers);

    // Set up quad vertex positions (unit quad centered at origin)
    s_data.quadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
    s_data.quadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
    s_data.quadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
    s_data.quadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};
}

void Renderer2D::shutdown() {
    delete[] s_data.quadVertexBufferBase;
    s_data.quadVertexBufferBase = nullptr;
    s_data.quadVertexBufferPtr = nullptr;

    s_data.quadVAO.reset();
    s_data.quadVBO.reset();
    s_data.quadIBO.reset();
    s_data.quadShader.reset();
    s_data.whiteTexture.reset();
}

void Renderer2D::beginScene(const OrthographicCamera& camera) {
    s_data.quadShader->bind();
    s_data.quadShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

    startBatch();
}

void Renderer2D::endScene() {
    flush();
}

void Renderer2D::startBatch() {
    s_data.quadIndexCount = 0;
    s_data.quadVertexBufferPtr = s_data.quadVertexBufferBase;
    s_data.textureSlotIndex = 1;
}

void Renderer2D::nextBatch() {
    flush();
    startBatch();
}

void Renderer2D::flush() {
    if (s_data.quadIndexCount == 0) {
        return;  // Nothing to draw
    }

    // Calculate data size
    u32 dataSize = static_cast<u32>(reinterpret_cast<u8*>(s_data.quadVertexBufferPtr) -
                                    reinterpret_cast<u8*>(s_data.quadVertexBufferBase));

    // Upload vertex data
    s_data.quadVAO->bind();
    glBindBuffer(GL_ARRAY_BUFFER, s_data.quadVAO->getVertexBuffers()[0].getNativeHandle());
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, s_data.quadVertexBufferBase);

    // Bind textures
    for (u32 i = 0; i < s_data.textureSlotIndex; ++i) {
        s_data.textureSlots[i]->bind(i);
    }

    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_data.quadIndexCount), GL_UNSIGNED_INT,
                   nullptr);
    s_data.stats.drawCalls++;
}

// ============================================================================
// Draw Quads - Position + Size
// ============================================================================

void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size,
                          const glm::vec4& color) {
    drawQuad({position.x, position.y, 0.0f}, size, color);
}

void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size,
                          const glm::vec4& color) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, color);
}

void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size,
                          const Texture2D& texture, f32 tilingFactor, const glm::vec4& tintColor) {
    drawQuad({position.x, position.y, 0.0f}, size, texture, tilingFactor, tintColor);
}

void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size,
                          const Texture2D& texture, f32 tilingFactor, const glm::vec4& tintColor) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size,
                          const Texture2D& texture, const glm::vec2& uvMin, const glm::vec2& uvMax,
                          const glm::vec4& tintColor) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, texture, uvMin, uvMax, tintColor);
}

// ============================================================================
// Draw Quads - Transform Matrix
// ============================================================================

void Renderer2D::drawQuad(const glm::mat4& transform, const glm::vec4& color) {
    constexpr usize quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    constexpr f32 textureIndex = 0.0f;  // White texture
    constexpr f32 tilingFactor = 1.0f;

    if (s_data.quadIndexCount >= MaxIndices) {
        nextBatch();
    }

    for (usize i = 0; i < quadVertexCount; ++i) {
        s_data.quadVertexBufferPtr->position = transform * s_data.quadVertexPositions[i];
        s_data.quadVertexBufferPtr->color = color;
        s_data.quadVertexBufferPtr->texCoord = textureCoords[i];
        s_data.quadVertexBufferPtr->texIndex = textureIndex;
        s_data.quadVertexBufferPtr->tilingFactor = tilingFactor;
        s_data.quadVertexBufferPtr++;
    }

    s_data.quadIndexCount += 6;
    s_data.stats.quadCount++;
}

void Renderer2D::drawQuad(const glm::mat4& transform, const Texture2D& texture, f32 tilingFactor,
                          const glm::vec4& tintColor) {
    constexpr usize quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    if (s_data.quadIndexCount >= MaxIndices) {
        nextBatch();
    }

    // Find or add texture to slots
    f32 textureIndex = 0.0f;
    for (u32 i = 1; i < s_data.textureSlotIndex; ++i) {
        if (s_data.textureSlots[i]->getNativeHandle() == texture.getNativeHandle()) {
            textureIndex = static_cast<f32>(i);
            break;
        }
    }

    if (textureIndex == 0.0f) {
        if (s_data.textureSlotIndex >= MaxTextureSlots) {
            nextBatch();
        }

        textureIndex = static_cast<f32>(s_data.textureSlotIndex);
        s_data.textureSlots[s_data.textureSlotIndex] = &texture;
        s_data.textureSlotIndex++;
    }

    for (usize i = 0; i < quadVertexCount; ++i) {
        s_data.quadVertexBufferPtr->position = transform * s_data.quadVertexPositions[i];
        s_data.quadVertexBufferPtr->color = tintColor;
        s_data.quadVertexBufferPtr->texCoord = textureCoords[i];
        s_data.quadVertexBufferPtr->texIndex = textureIndex;
        s_data.quadVertexBufferPtr->tilingFactor = tilingFactor;
        s_data.quadVertexBufferPtr++;
    }

    s_data.quadIndexCount += 6;
    s_data.stats.quadCount++;
}

void Renderer2D::drawQuad(const glm::mat4& transform, const Texture2D& texture,
                          const glm::vec2& uvMin, const glm::vec2& uvMax,
                          const glm::vec4& tintColor) {
    constexpr usize quadVertexCount = 4;
    // Custom UV coordinates for sprite sheet support
    const glm::vec2 textureCoords[] = {
        {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}};

    if (s_data.quadIndexCount >= MaxIndices) {
        nextBatch();
    }

    // Find or add texture to slots
    f32 textureIndex = 0.0f;
    for (u32 i = 1; i < s_data.textureSlotIndex; ++i) {
        if (s_data.textureSlots[i]->getNativeHandle() == texture.getNativeHandle()) {
            textureIndex = static_cast<f32>(i);
            break;
        }
    }

    if (textureIndex == 0.0f) {
        if (s_data.textureSlotIndex >= MaxTextureSlots) {
            nextBatch();
        }

        textureIndex = static_cast<f32>(s_data.textureSlotIndex);
        s_data.textureSlots[s_data.textureSlotIndex] = &texture;
        s_data.textureSlotIndex++;
    }

    for (usize i = 0; i < quadVertexCount; ++i) {
        s_data.quadVertexBufferPtr->position = transform * s_data.quadVertexPositions[i];
        s_data.quadVertexBufferPtr->color = tintColor;
        s_data.quadVertexBufferPtr->texCoord = textureCoords[i];
        s_data.quadVertexBufferPtr->texIndex = textureIndex;
        s_data.quadVertexBufferPtr->tilingFactor = 1.0f;
        s_data.quadVertexBufferPtr++;
    }

    s_data.quadIndexCount += 6;
    s_data.stats.quadCount++;
}

// ============================================================================
// Draw Rotated Quads
// ============================================================================

void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, f32 rotation,
                                 const glm::vec4& color) {
    drawRotatedQuad({position.x, position.y, 0.0f}, size, rotation, color);
}

void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                                 const glm::vec4& color) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, color);
}

void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, f32 rotation,
                                 const Texture2D& texture, f32 tilingFactor,
                                 const glm::vec4& tintColor) {
    drawRotatedQuad({position.x, position.y, 0.0f}, size, rotation, texture, tilingFactor,
                    tintColor);
}

void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                                 const Texture2D& texture, f32 tilingFactor,
                                 const glm::vec4& tintColor) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, texture, tilingFactor, tintColor);
}

// ============================================================================
// Statistics
// ============================================================================

Renderer2D::Statistics Renderer2D::getStats() {
    return s_data.stats;
}

void Renderer2D::resetStats() {
    s_data.stats.drawCalls = 0;
    s_data.stats.quadCount = 0;
}

}  // namespace limbo
