#include "limbo/render/2d/TextRenderer.hpp"
#include "limbo/render/2d/Renderer2D.hpp"

namespace limbo {

TextRenderer::Statistics TextRenderer::s_stats;

void TextRenderer::drawText(const String& text, const glm::vec2& position, const Font& font,
                            f32 scale, const glm::vec4& color) {
    drawText(text, glm::vec3(position, 0.0f), font, scale, color);
}

void TextRenderer::drawText(const String& text, const glm::vec3& position, const Font& font,
                            f32 scale, const glm::vec4& color) {
    if (text.empty() || font.getAtlas() == nullptr) {
        return;
    }

    const Texture2D* atlas = font.getAtlas();
    f32 x = position.x;
    f32 y = position.y;
    f32 z = position.z;

    for (char c : text) {
        // Handle newlines
        if (c == '\n') {
            x = position.x;
            y += font.getLineHeight() * scale;
            continue;
        }

        // Skip non-printable characters (except space)
        if (c < 32) {
            continue;
        }

        const Glyph& glyph = font.getGlyph(c);

        // Skip empty glyphs (like space)
        if (glyph.size.x > 0 && glyph.size.y > 0) {
            // Calculate quad position
            // bearing.x is offset from cursor to left edge of glyph
            // bearing.y is offset from baseline to top of glyph (negative = below baseline)
            f32 xpos = x + glyph.bearing.x * scale;
            f32 ypos = y + glyph.bearing.y * scale;

            f32 w = glyph.size.x * scale;
            f32 h = glyph.size.y * scale;

            // Draw the glyph quad
            // Position is center of quad, so offset by half size
            glm::vec3 quadPos(xpos + w * 0.5f, ypos + h * 0.5f, z);

            Renderer2D::drawQuad(quadPos, glm::vec2(w, h), *atlas, glyph.uvMin, glyph.uvMax, color);

            s_stats.glyphsRendered++;
        }

        // Advance cursor
        x += glyph.advance * scale;
    }
}

glm::vec2 TextRenderer::measureText(const String& text, const Font& font, f32 scale) {
    if (text.empty()) {
        return glm::vec2(0.0f);
    }

    f32 maxWidth = 0.0f;
    f32 currentWidth = 0.0f;
    f32 height = font.getLineHeight() * scale;
    i32 lineCount = 1;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, currentWidth);
            currentWidth = 0.0f;
            lineCount++;
            continue;
        }

        if (c < 32) {
            continue;
        }

        const Glyph& glyph = font.getGlyph(c);
        currentWidth += glyph.advance * scale;
    }

    maxWidth = std::max(maxWidth, currentWidth);

    return glm::vec2(maxWidth, height * static_cast<f32>(lineCount));
}

TextRenderer::Statistics TextRenderer::getStats() {
    return s_stats;
}

void TextRenderer::resetStats() {
    s_stats = Statistics{};
}

}  // namespace limbo
