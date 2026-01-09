#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/render/2d/Font.hpp"

#include <glm/glm.hpp>

namespace limbo {

/**
 * TextRenderer - Static text rendering API
 *
 * Renders text using bitmap fonts through the Renderer2D batching system.
 * Each character is rendered as a textured quad.
 *
 * Usage:
 *   // In render loop (between Renderer2D::beginScene and endScene)
 *   TextRenderer::drawText("Hello World", {10, 100}, font, 1.0f, {1, 1, 1, 1});
 */
class LIMBO_API TextRenderer {
public:
    /**
     * Draw text at a position
     * @param text The text string to render
     * @param position Top-left position in world coordinates
     * @param font The font to use
     * @param scale Text scale factor (default 1.0)
     * @param color Text color (default white)
     */
    static void drawText(const String& text, const glm::vec2& position, const Font& font,
                         f32 scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f));

    /**
     * Draw text at a 3D position
     * @param text The text string to render
     * @param position Position in world coordinates
     * @param font The font to use
     * @param scale Text scale factor (default 1.0)
     * @param color Text color (default white)
     */
    static void drawText(const String& text, const glm::vec3& position, const Font& font,
                         f32 scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f));

    /**
     * Measure the size of a text string
     * @param text The text to measure
     * @param font The font to use
     * @param scale Text scale factor (default 1.0)
     * @return Size of the text bounding box (width, height)
     */
    [[nodiscard]] static glm::vec2 measureText(const String& text, const Font& font,
                                               f32 scale = 1.0f);

    /**
     * Get text rendering statistics
     */
    struct Statistics {
        u32 glyphsRendered = 0;
    };

    [[nodiscard]] static Statistics getStats();
    static void resetStats();

private:
    static Statistics s_stats;
};

}  // namespace limbo
