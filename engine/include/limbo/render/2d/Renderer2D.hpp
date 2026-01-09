#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/render/common/Texture.hpp"

#include <glm/glm.hpp>

#include <array>

namespace limbo {

// Forward declarations
class Shader;
class VertexArray;
class VertexBuffer;
class IndexBuffer;
class OrthographicCamera;

/**
 * Renderer2D - Batched 2D sprite renderer
 *
 * Efficiently renders many 2D quads (sprites) by batching them into
 * a single draw call. Supports colored quads, textured quads, and
 * rotated/scaled transforms.
 *
 * Usage:
 *   Renderer2D::init();
 *   ...
 *   Renderer2D::beginScene(camera);
 *   Renderer2D::drawQuad({0, 0}, {1, 1}, {1, 0, 0, 1}); // Red quad
 *   Renderer2D::drawQuad({2, 0}, {1, 1}, texture);      // Textured quad
 *   Renderer2D::endScene();
 *   ...
 *   Renderer2D::shutdown();
 */
class LIMBO_API Renderer2D {
public:
    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * Initialize the 2D renderer
     * Must be called before any other Renderer2D functions
     */
    static void init();

    /**
     * Shutdown the 2D renderer and release resources
     */
    static void shutdown();

    // ========================================================================
    // Scene Management
    // ========================================================================

    /**
     * Begin a new scene for rendering
     * @param camera The camera providing the view-projection matrix
     */
    static void beginScene(const OrthographicCamera& camera);

    /**
     * End the current scene and flush all batched geometry
     */
    static void endScene();

    /**
     * Flush the current batch (renders all queued quads)
     * Called automatically by endScene, but can be called manually
     */
    static void flush();

    // ========================================================================
    // Draw Primitives - Position + Size
    // ========================================================================

    /**
     * Draw a colored quad
     * @param position 2D position (z = 0)
     * @param size Width and height
     * @param color RGBA color
     */
    static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

    /**
     * Draw a colored quad with z-depth
     * @param position 3D position
     * @param size Width and height
     * @param color RGBA color
     */
    static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);

    /**
     * Draw a textured quad
     * @param position 2D position (z = 0)
     * @param size Width and height
     * @param texture The texture to use
     * @param tilingFactor Texture repeat factor (default 1.0)
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawQuad(const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
                         f32 tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * Draw a textured quad with z-depth
     * @param position 3D position
     * @param size Width and height
     * @param texture The texture to use
     * @param tilingFactor Texture repeat factor (default 1.0)
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawQuad(const glm::vec3& position, const glm::vec2& size, const Texture2D& texture,
                         f32 tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * Draw a textured quad with custom UV coordinates (for tilemaps/sprite sheets)
     * @param position 3D position
     * @param size Width and height
     * @param texture The texture to use
     * @param uvMin Bottom-left UV coordinate
     * @param uvMax Top-right UV coordinate
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawQuad(const glm::vec3& position, const glm::vec2& size, const Texture2D& texture,
                         const glm::vec2& uvMin, const glm::vec2& uvMax,
                         const glm::vec4& tintColor = glm::vec4(1.0f));

    // ========================================================================
    // Draw Primitives - Transform Matrix
    // ========================================================================

    /**
     * Draw a colored quad with a transform matrix
     * @param transform The model transform matrix
     * @param color RGBA color
     */
    static void drawQuad(const glm::mat4& transform, const glm::vec4& color);

    /**
     * Draw a textured quad with a transform matrix
     * @param transform The model transform matrix
     * @param texture The texture to use
     * @param tilingFactor Texture repeat factor (default 1.0)
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawQuad(const glm::mat4& transform, const Texture2D& texture,
                         f32 tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * Draw a textured quad with custom UV coordinates (for sprite sheets)
     * @param transform The model transform matrix
     * @param texture The texture to use
     * @param uvMin Bottom-left UV coordinate
     * @param uvMax Top-right UV coordinate
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawQuad(const glm::mat4& transform, const Texture2D& texture,
                         const glm::vec2& uvMin, const glm::vec2& uvMax,
                         const glm::vec4& tintColor = glm::vec4(1.0f));

    // ========================================================================
    // Draw Primitives - Rotated Quads
    // ========================================================================

    /**
     * Draw a rotated colored quad
     * @param position 2D position (z = 0)
     * @param size Width and height
     * @param rotation Rotation in radians
     * @param color RGBA color
     */
    static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, f32 rotation,
                                const glm::vec4& color);

    /**
     * Draw a rotated colored quad with z-depth
     * @param position 3D position
     * @param size Width and height
     * @param rotation Rotation in radians
     * @param color RGBA color
     */
    static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                                const glm::vec4& color);

    /**
     * Draw a rotated textured quad
     * @param position 2D position (z = 0)
     * @param size Width and height
     * @param rotation Rotation in radians
     * @param texture The texture to use
     * @param tilingFactor Texture repeat factor (default 1.0)
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, f32 rotation,
                                const Texture2D& texture, f32 tilingFactor = 1.0f,
                                const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * Draw a rotated textured quad with z-depth
     * @param position 3D position
     * @param size Width and height
     * @param rotation Rotation in radians
     * @param texture The texture to use
     * @param tilingFactor Texture repeat factor (default 1.0)
     * @param tintColor Color to multiply with texture (default white)
     */
    static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                                const Texture2D& texture, f32 tilingFactor = 1.0f,
                                const glm::vec4& tintColor = glm::vec4(1.0f));

    // ========================================================================
    // Debug Primitives - Lines
    // ========================================================================

    /**
     * Draw a line between two points
     * @param p0 Start point
     * @param p1 End point
     * @param color Line color
     */
    static void drawLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& color);

    /**
     * Draw a line between two 3D points
     * @param p0 Start point
     * @param p1 End point
     * @param color Line color
     */
    static void drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

    // ========================================================================
    // Debug Primitives - Shapes (Wireframe)
    // ========================================================================

    /**
     * Draw a wireframe rectangle
     * @param position Center position
     * @param size Width and height
     * @param color Line color
     */
    static void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

    /**
     * Draw a wireframe rectangle with rotation
     * @param position Center position
     * @param size Width and height
     * @param rotation Rotation in radians
     * @param color Line color
     */
    static void drawRect(const glm::vec2& position, const glm::vec2& size, f32 rotation,
                         const glm::vec4& color);

    /**
     * Draw a wireframe circle
     * @param center Circle center
     * @param radius Circle radius
     * @param color Line color
     * @param segments Number of segments (default 32)
     */
    static void drawCircle(const glm::vec2& center, f32 radius, const glm::vec4& color,
                           i32 segments = 32);

    /**
     * Draw a wireframe circle with thickness (filled ring)
     * @param center Circle center
     * @param radius Circle radius
     * @param thickness Ring thickness
     * @param color Color
     * @param segments Number of segments (default 32)
     */
    static void drawCircle(const glm::vec2& center, f32 radius, f32 thickness,
                           const glm::vec4& color, i32 segments = 32);

    // ========================================================================
    // Debug Primitives - Filled Shapes
    // ========================================================================

    /**
     * Draw a filled circle
     * @param center Circle center
     * @param radius Circle radius
     * @param color Fill color
     * @param segments Number of segments (default 32)
     */
    static void drawFilledCircle(const glm::vec2& center, f32 radius, const glm::vec4& color,
                                 i32 segments = 32);

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Statistics {
        u32 drawCalls = 0;
        u32 quadCount = 0;
        u32 lineCount = 0;
        u32 textureBinds = 0;
        u32 batchCount = 0;

        [[nodiscard]] u32 vertexCount() const { return quadCount * 4 + lineCount * 2; }
        [[nodiscard]] u32 indexCount() const { return quadCount * 6; }
    };

    /**
     * Get rendering statistics for the current frame
     */
    static Statistics getStats();

    /**
     * Reset statistics (call at start of frame)
     */
    static void resetStats();

private:
    static void startBatch();
    static void nextBatch();
};

}  // namespace limbo
