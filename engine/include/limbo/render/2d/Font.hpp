#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/render/common/Texture.hpp"

#include <glm/glm.hpp>

#include <filesystem>
#include <unordered_map>

namespace limbo {

/**
 * Glyph - Information about a single character in a font
 */
struct Glyph {
    glm::vec2 uvMin{0.0f};    // Bottom-left UV coordinate in atlas
    glm::vec2 uvMax{0.0f};    // Top-right UV coordinate in atlas
    glm::vec2 size{0.0f};     // Glyph size in pixels
    glm::vec2 bearing{0.0f};  // Offset from baseline to top-left of glyph
    f32 advance = 0.0f;       // Horizontal advance to next glyph
};

/**
 * Font - Bitmap font loaded from TrueType files
 *
 * Uses stb_truetype to rasterize glyphs into a texture atlas.
 * Supports ASCII characters (32-126) by default.
 *
 * Usage:
 *   auto fontResult = Font::loadFromFile("assets/fonts/arial.ttf", 32.0f);
 *   if (fontResult) {
 *       Font& font = *fontResult.value();
 *       TextRenderer::drawText("Hello", {0, 0}, font);
 *   }
 */
class LIMBO_API Font {
public:
    Font() = default;
    ~Font() = default;

    // Non-copyable, movable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = default;
    Font& operator=(Font&&) = default;

    /**
     * Load a font from a TrueType file
     * @param path Path to the .ttf file
     * @param fontSize Font size in pixels
     * @param firstChar First ASCII character to include (default 32 = space)
     * @param charCount Number of characters to include (default 95 = printable ASCII)
     * @return Result containing the font or an error message
     */
    static Result<Unique<Font>, String> loadFromFile(const std::filesystem::path& path,
                                                     f32 fontSize, i32 firstChar = 32,
                                                     i32 charCount = 95);

    /**
     * Get glyph information for a character
     * @param c The character to look up
     * @return Reference to the glyph data (returns a default glyph for unknown chars)
     */
    [[nodiscard]] const Glyph& getGlyph(char c) const;

    /**
     * Get the font's texture atlas
     */
    [[nodiscard]] const Texture2D* getAtlas() const { return m_atlas.get(); }

    /**
     * Get the font size in pixels
     */
    [[nodiscard]] f32 getFontSize() const { return m_fontSize; }

    /**
     * Get the line height (distance between baselines)
     */
    [[nodiscard]] f32 getLineHeight() const { return m_lineHeight; }

    /**
     * Get the ascent (distance from baseline to top of tallest glyph)
     */
    [[nodiscard]] f32 getAscent() const { return m_ascent; }

    /**
     * Get the descent (distance from baseline to bottom of lowest glyph)
     */
    [[nodiscard]] f32 getDescent() const { return m_descent; }

private:
    Unique<Texture2D> m_atlas;
    std::unordered_map<char, Glyph> m_glyphs;
    f32 m_fontSize = 0.0f;
    f32 m_lineHeight = 0.0f;
    f32 m_ascent = 0.0f;
    f32 m_descent = 0.0f;
    i32 m_firstChar = 32;
    Glyph m_defaultGlyph;  // Returned for unknown characters
};

}  // namespace limbo
