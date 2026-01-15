#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/common/Shader.hpp"
#include "limbo/render/common/Buffer.hpp"
#include "limbo/render/common/Camera.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace limbo {

// Maximum quads per batch
static constexpr u32 MaxQuads = 10000;
static constexpr u32 MaxVertices = MaxQuads * 4;
static constexpr u32 MaxIndices = MaxQuads * 6;
static u32 s_MaxTextureSlots = 0;

// Maximum lines per batch
static constexpr u32 MaxLines = 10000;
static constexpr u32 MaxLineVertices = MaxLines * 2;

struct QuadVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    f32 texIndex;
    f32 tilingFactor;
};

struct LineVertex {
    glm::vec3 position;
    glm::vec4 color;
};

struct Renderer2DData {
    // Quad batch data
    Unique<VertexArray> quadVAO;
    Unique<VertexBuffer> quadVBO;
    Unique<IndexBuffer> quadIBO;
    Unique<Shader> quadShader;
    Unique<Texture2D> whiteTexture;

    u32 quadIndexCount = 0;
    QuadVertex* quadVertexBufferBase = nullptr;
    QuadVertex* quadVertexBufferPtr = nullptr;

    std::vector<const Texture2D*> textureSlots;
    u32 textureSlotIndex = 1;  // 0 = white texture

    glm::vec4 quadVertexPositions[4];

    // Line batch data
    Unique<VertexArray> lineVAO;
    Unique<VertexBuffer> lineVBO;
    Unique<Shader> lineShader;

    u32 lineVertexCount = 0;
    LineVertex* lineVertexBufferBase = nullptr;
    LineVertex* lineVertexBufferPtr = nullptr;

    Renderer2D::Statistics stats;
};

static Renderer2DData s_data;

void Renderer2D::init() {
    // Query max texture slots
    i32 maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    s_MaxTextureSlots = std::min(static_cast<u32>(maxTextureUnits), 32u);

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
    s_data.textureSlots.resize(s_MaxTextureSlots);
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

    std::string fragmentSource = R"(
        #version 450 core

        in vec4 v_Color;
        in vec2 v_TexCoord;
        in flat float v_TexIndex;
        in float v_TilingFactor;

        uniform sampler2D u_Textures[)";
    fragmentSource += std::to_string(s_MaxTextureSlots);
    fragmentSource += R"(];

        out vec4 o_Color;

        void main() {
            int index = int(v_TexIndex);
            vec4 texColor = texture(u_Textures[index], v_TexCoord * v_TilingFactor);
            o_Color = texColor * v_Color;
        }
    )";

    auto result = s_data.quadShader->loadFromSource(vertexSource, fragmentSource.c_str());
    if (!result) {
        // Log error but continue - will just render nothing
    }

    // Set up texture sampler array
    s_data.quadShader->bind();
    std::vector<i32> samplers(s_MaxTextureSlots);
    for (u32 i = 0; i < s_MaxTextureSlots; ++i) {
        samplers[i] = static_cast<i32>(i);
    }
    glUniform1iv(glGetUniformLocation(s_data.quadShader->getNativeHandle(), "u_Textures"),
                 static_cast<GLsizei>(s_MaxTextureSlots), samplers.data());

    // Set up quad vertex positions (unit quad centered at origin)
    s_data.quadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
    s_data.quadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
    s_data.quadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
    s_data.quadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};

    // ========================================================================
    // Line Rendering Setup
    // ========================================================================

    // Create line VAO
    s_data.lineVAO = make_unique<VertexArray>();
    s_data.lineVAO->create();

    // Create line VBO
    s_data.lineVBO = make_unique<VertexBuffer>();
    s_data.lineVBO->create(nullptr, MaxLineVertices * sizeof(LineVertex));
    s_data.lineVBO->setLayout(
        {{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float4, "a_Color"}});
    s_data.lineVAO->addVertexBuffer(std::move(*s_data.lineVBO));

    // Allocate CPU-side line vertex buffer
    s_data.lineVertexBufferBase = new LineVertex[MaxLineVertices];

    // Create line shader
    s_data.lineShader = make_unique<Shader>();

    const char* lineVertexSource = R"(
        #version 450 core

        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec4 a_Color;

        uniform mat4 u_ViewProjection;

        out vec4 v_Color;

        void main() {
            v_Color = a_Color;
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

    const char* lineFragmentSource = R"(
        #version 450 core

        in vec4 v_Color;

        out vec4 o_Color;

        void main() {
            o_Color = v_Color;
        }
    )";

    auto lineResult = s_data.lineShader->loadFromSource(lineVertexSource, lineFragmentSource);
    if (!lineResult) {
        // Log error but continue
    }
}

void Renderer2D::shutdown() {
    delete[] s_data.quadVertexBufferBase;
    s_data.quadVertexBufferBase = nullptr;
    s_data.quadVertexBufferPtr = nullptr;

    delete[] s_data.lineVertexBufferBase;
    s_data.lineVertexBufferBase = nullptr;
    s_data.lineVertexBufferPtr = nullptr;

    s_data.quadVAO.reset();
    s_data.quadVBO.reset();
    s_data.quadIBO.reset();
    s_data.quadShader.reset();
    s_data.whiteTexture.reset();

    s_data.lineVAO.reset();
    s_data.lineVBO.reset();
    s_data.lineShader.reset();
}

void Renderer2D::beginScene(const OrthographicCamera& camera) {
    s_data.quadShader->bind();
    s_data.quadShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

    s_data.lineShader->bind();
    s_data.lineShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

    startBatch();
}

void Renderer2D::endScene() {
    flush();
}

void Renderer2D::startBatch() {
    s_data.quadIndexCount = 0;
    s_data.quadVertexBufferPtr = s_data.quadVertexBufferBase;
    s_data.textureSlotIndex = 1;

    s_data.lineVertexCount = 0;
    s_data.lineVertexBufferPtr = s_data.lineVertexBufferBase;

    s_data.stats.batchCount++;
}

void Renderer2D::nextBatch() {
    flush();
    startBatch();
}

void Renderer2D::flush() {
    // Flush quads
    if (s_data.quadIndexCount > 0) {
        // Calculate data size
        u32 const dataSize = static_cast<u32>(reinterpret_cast<u8*>(s_data.quadVertexBufferPtr) -
                                              reinterpret_cast<u8*>(s_data.quadVertexBufferBase));

        // Upload vertex data
        s_data.quadVAO->bind();
        glBindBuffer(GL_ARRAY_BUFFER, s_data.quadVAO->getVertexBuffers()[0].getNativeHandle());
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, s_data.quadVertexBufferBase);

        // Bind textures
        for (u32 i = 0; i < s_data.textureSlotIndex; ++i) {
            s_data.textureSlots[i]->bind(i);
            s_data.stats.textureBinds++;
        }

        // Draw quads
        s_data.quadShader->bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_data.quadIndexCount), GL_UNSIGNED_INT,
                       nullptr);
        s_data.stats.drawCalls++;

        // Reset quad buffer
        s_data.quadIndexCount = 0;
        s_data.quadVertexBufferPtr = s_data.quadVertexBufferBase;
        s_data.textureSlotIndex = 1;
    }

    // Flush lines
    if (s_data.lineVertexCount > 0) {
        // Calculate data size
        u32 const lineDataSize =
            static_cast<u32>(reinterpret_cast<u8*>(s_data.lineVertexBufferPtr) -
                             reinterpret_cast<u8*>(s_data.lineVertexBufferBase));

        // Upload vertex data
        s_data.lineVAO->bind();
        glBindBuffer(GL_ARRAY_BUFFER, s_data.lineVAO->getVertexBuffers()[0].getNativeHandle());
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineDataSize, s_data.lineVertexBufferBase);

        // Draw lines
        s_data.lineShader->bind();
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(s_data.lineVertexCount));
        s_data.stats.drawCalls++;

        // Reset line buffer
        s_data.lineVertexCount = 0;
        s_data.lineVertexBufferPtr = s_data.lineVertexBufferBase;
    }
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
    glm::mat4 const transform = glm::translate(glm::mat4(1.0f), position) *
                                glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, color);
}

void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size,
                          const Texture2D& texture, f32 tilingFactor, const glm::vec4& tintColor) {
    drawQuad({position.x, position.y, 0.0f}, size, texture, tilingFactor, tintColor);
}

void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size,
                          const Texture2D& texture, f32 tilingFactor, const glm::vec4& tintColor) {
    glm::mat4 const transform = glm::translate(glm::mat4(1.0f), position) *
                                glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size,
                          const Texture2D& texture, const glm::vec2& uvMin, const glm::vec2& uvMax,
                          const glm::vec4& tintColor) {
    glm::mat4 const transform = glm::translate(glm::mat4(1.0f), position) *
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
        if (s_data.textureSlotIndex >= s_MaxTextureSlots) {
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
        if (s_data.textureSlotIndex >= s_MaxTextureSlots) {
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
    glm::mat4 const transform = glm::translate(glm::mat4(1.0f), position) *
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
    glm::mat4 const transform = glm::translate(glm::mat4(1.0f), position) *
                                glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f}) *
                                glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    drawQuad(transform, texture, tilingFactor, tintColor);
}

// ============================================================================
// Immediate Mode Drawing
// ============================================================================

void Renderer2D::drawQuadImmediate(const glm::mat4& transform, const Texture2D* texture,
                                   const glm::vec4& color) {
    constexpr usize quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    constexpr f32 textureIndex = 0.0f;
    constexpr f32 tilingFactor = 1.0f;

    // Build vertex data for a single quad
    QuadVertex vertices[4];
    for (usize i = 0; i < quadVertexCount; ++i) {
        vertices[i].position = transform * s_data.quadVertexPositions[i];
        vertices[i].color = color;
        vertices[i].texCoord = textureCoords[i];
        vertices[i].texIndex = textureIndex;
        vertices[i].tilingFactor = tilingFactor;
    }

    // Upload vertex data
    s_data.quadVAO->bind();
    glBindBuffer(GL_ARRAY_BUFFER, s_data.quadVAO->getVertexBuffers()[0].getNativeHandle());
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    // Bind texture if provided, otherwise use white texture
    if (texture != nullptr) {
        texture->bind(0);
    } else {
        s_data.whiteTexture->bind(0);
    }

    // Draw the quad (shader is already bound by caller)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    s_data.stats.drawCalls++;
    s_data.stats.quadCount++;
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
    s_data.stats.lineCount = 0;
    s_data.stats.textureBinds = 0;
    s_data.stats.batchCount = 0;
}

// ============================================================================
// Draw Lines
// ============================================================================

void Renderer2D::drawLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& color) {
    drawLine(glm::vec3(p0, 0.0f), glm::vec3(p1, 0.0f), color);
}

void Renderer2D::drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color) {
    if (s_data.lineVertexCount >= MaxLineVertices) {
        nextBatch();
    }

    s_data.lineVertexBufferPtr->position = p0;
    s_data.lineVertexBufferPtr->color = color;
    s_data.lineVertexBufferPtr++;

    s_data.lineVertexBufferPtr->position = p1;
    s_data.lineVertexBufferPtr->color = color;
    s_data.lineVertexBufferPtr++;

    s_data.lineVertexCount += 2;
    s_data.stats.lineCount++;
}

// ============================================================================
// Draw Shapes (Wireframe)
// ============================================================================

void Renderer2D::drawRect(const glm::vec2& position, const glm::vec2& size,
                          const glm::vec4& color) {
    drawRect(position, size, 0.0f, color);
}

void Renderer2D::drawRect(const glm::vec2& position, const glm::vec2& size, f32 rotation,
                          const glm::vec4& color) {
    drawRect(glm::vec3(position, 0.0f), size, rotation, color);
}

void Renderer2D::drawRect(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                          const glm::vec4& color) {
    glm::vec2 const halfSize = size * 0.5f;

    // Calculate corners
    glm::vec2 corners[4] = {{-halfSize.x, -halfSize.y},
                            {halfSize.x, -halfSize.y},
                            {halfSize.x, halfSize.y},
                            {-halfSize.x, halfSize.y}};

    // Apply rotation if needed
    if (rotation != 0.0f) {
        f32 const c = std::cos(rotation);
        f32 const s = std::sin(rotation);
        for (auto& corner : corners) {
            f32 const x = corner.x * c - corner.y * s;
            f32 const y = corner.x * s + corner.y * c;
            corner = {x, y};
        }
    }

    // Offset by position (x, y only)
    for (auto& corner : corners) {
        corner.x += position.x;
        corner.y += position.y;
    }

    // Draw four lines with z-depth
    f32 const z = position.z;
    drawLine(glm::vec3(corners[0], z), glm::vec3(corners[1], z), color);
    drawLine(glm::vec3(corners[1], z), glm::vec3(corners[2], z), color);
    drawLine(glm::vec3(corners[2], z), glm::vec3(corners[3], z), color);
    drawLine(glm::vec3(corners[3], z), glm::vec3(corners[0], z), color);
}

void Renderer2D::drawCircle(const glm::vec2& center, f32 radius, const glm::vec4& color,
                            i32 segments) {
    drawCircle(glm::vec3(center, 0.0f), radius, color, segments);
}

void Renderer2D::drawCircle(const glm::vec3& center, f32 radius, const glm::vec4& color,
                            i32 segments) {
    if (segments < 3) {
        segments = 3;
    }

    f32 const angleStep = glm::two_pi<f32>() / static_cast<f32>(segments);
    f32 const z = center.z;

    glm::vec2 prevPoint = glm::vec2(center) + glm::vec2(radius, 0.0f);

    for (i32 i = 1; i <= segments; ++i) {
        f32 const angle = angleStep * static_cast<f32>(i);
        glm::vec2 const currentPoint =
            glm::vec2(center) + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
        drawLine(glm::vec3(prevPoint, z), glm::vec3(currentPoint, z), color);
        prevPoint = currentPoint;
    }
}

void Renderer2D::drawCircle(const glm::vec2& center, f32 radius, f32 thickness,
                            const glm::vec4& color, i32 segments) {
    // Draw as a thick ring using quads
    if (segments < 3) {
        segments = 3;
    }

    f32 const innerRadius = radius - thickness * 0.5f;
    f32 const outerRadius = radius + thickness * 0.5f;
    f32 const angleStep = glm::two_pi<f32>() / static_cast<f32>(segments);

    for (i32 i = 0; i < segments; ++i) {
        f32 const angle0 = angleStep * static_cast<f32>(i);
        f32 const angle1 = angleStep * static_cast<f32>(i + 1);

        glm::vec2 const inner0 =
            center + glm::vec2(std::cos(angle0), std::sin(angle0)) * innerRadius;
        glm::vec2 const outer0 =
            center + glm::vec2(std::cos(angle0), std::sin(angle0)) * outerRadius;
        glm::vec2 const inner1 =
            center + glm::vec2(std::cos(angle1), std::sin(angle1)) * innerRadius;
        glm::vec2 const outer1 =
            center + glm::vec2(std::cos(angle1), std::sin(angle1)) * outerRadius;

        // Draw as two triangles (using quads)
        // For now, approximate with lines on inner and outer edges
        drawLine(inner0, inner1, color);
        drawLine(outer0, outer1, color);
    }
}

// ============================================================================
// Draw Filled Shapes
// ============================================================================

void Renderer2D::drawFilledCircle(const glm::vec2& center, f32 radius, const glm::vec4& color,
                                  i32 segments) {
    if (segments < 3) {
        segments = 3;
    }

    f32 const angleStep = glm::two_pi<f32>() / static_cast<f32>(segments);

    // Draw as triangle fan using quads
    // Each segment is a triangle from center to two adjacent edge points
    for (i32 i = 0; i < segments; ++i) {
        f32 const angle0 = angleStep * static_cast<f32>(i);
        f32 const angle1 = angleStep * static_cast<f32>(i + 1);

        glm::vec2 const p0 = center + glm::vec2(std::cos(angle0), std::sin(angle0)) * radius;
        glm::vec2 const p1 = center + glm::vec2(std::cos(angle1), std::sin(angle1)) * radius;

        // Draw triangle as a very thin quad from center to edge
        // For a proper filled circle, we'd need triangle primitives
        // Approximate by drawing many thin quads radiating from center
        glm::vec2 const mid = (p0 + p1) * 0.5f;
        glm::vec2 const dir = glm::normalize(mid - center);
        glm::vec2 const size = glm::vec2(glm::length(p1 - p0), radius);

        f32 const rotation = std::atan2(dir.y, dir.x);
        glm::vec2 const quadCenter = center + dir * (radius * 0.5f);

        // Use rotated quad
        glm::mat4 const transform =
            glm::translate(glm::mat4(1.0f), glm::vec3(quadCenter, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(radius, size.x, 1.0f));

        drawQuad(transform, color);
    }
}

}  // namespace limbo
