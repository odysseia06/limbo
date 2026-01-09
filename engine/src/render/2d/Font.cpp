#include "limbo/render/2d/Font.hpp"
#include "limbo/util/FileIO.hpp"

#include <spdlog/spdlog.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <cmath>

namespace limbo {

Result<Unique<Font>, String> Font::loadFromFile(const std::filesystem::path& path, f32 fontSize,
                                                i32 firstChar, i32 charCount) {
    // Read the font file
    auto fileResult = util::readFileBinary(path);
    if (!fileResult) {
        return unexpected<String>("Failed to read font file: " + path.string());
    }

    const auto& fontData = fileResult.value();

    // Initialize stb_truetype
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontData.data(), 0)) {
        return unexpected<String>("Failed to parse font file: " + path.string());
    }

    // Calculate scale factor
    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

    // Get font metrics
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    auto font = std::make_unique<Font>();
    font->m_fontSize = fontSize;
    font->m_ascent = static_cast<f32>(ascent) * scale;
    font->m_descent = static_cast<f32>(descent) * scale;
    font->m_lineHeight = (static_cast<f32>(ascent - descent + lineGap)) * scale;
    font->m_firstChar = firstChar;

    // Calculate atlas size (start with reasonable size and grow if needed)
    i32 atlasWidth = 512;
    i32 atlasHeight = 512;

    // Allocate atlas bitmap
    std::vector<u8> atlasBitmap(static_cast<usize>(atlasWidth * atlasHeight), 0);

    // Pack characters into atlas using stb_truetype's packing API
    stbtt_pack_context packContext;
    if (!stbtt_PackBegin(&packContext, atlasBitmap.data(), atlasWidth, atlasHeight, 0, 1,
                         nullptr)) {
        return unexpected<String>("Failed to initialize font packing");
    }

    // Set oversampling for better quality
    stbtt_PackSetOversampling(&packContext, 2, 2);

    // Pack character range
    std::vector<stbtt_packedchar> packedChars(static_cast<usize>(charCount));
    if (!stbtt_PackFontRange(&packContext, fontData.data(), 0, fontSize, firstChar, charCount,
                             packedChars.data())) {
        stbtt_PackEnd(&packContext);
        return unexpected<String>("Failed to pack font glyphs - atlas may be too small");
    }

    stbtt_PackEnd(&packContext);

    // Create glyph map from packed data
    for (i32 i = 0; i < charCount; ++i) {
        char c = static_cast<char>(firstChar + i);
        const auto& pc = packedChars[static_cast<usize>(i)];

        Glyph glyph;
        glyph.uvMin =
            glm::vec2(pc.x0 / static_cast<f32>(atlasWidth), pc.y0 / static_cast<f32>(atlasHeight));
        glyph.uvMax =
            glm::vec2(pc.x1 / static_cast<f32>(atlasWidth), pc.y1 / static_cast<f32>(atlasHeight));
        glyph.size = glm::vec2(pc.x1 - pc.x0, pc.y1 - pc.y0);
        glyph.bearing = glm::vec2(pc.xoff, pc.yoff);
        glyph.advance = pc.xadvance;

        font->m_glyphs[c] = glyph;
    }

    // Set up default glyph (use '?' or space)
    if (font->m_glyphs.count('?') > 0) {
        font->m_defaultGlyph = font->m_glyphs['?'];
    } else if (font->m_glyphs.count(' ') > 0) {
        font->m_defaultGlyph = font->m_glyphs[' '];
    }

    // Create texture from atlas (convert single-channel to RGBA)
    std::vector<u8> rgbaData(static_cast<usize>(atlasWidth * atlasHeight * 4));
    for (i32 i = 0; i < atlasWidth * atlasHeight; ++i) {
        usize idx = static_cast<usize>(i);
        rgbaData[idx * 4 + 0] = 255;               // R
        rgbaData[idx * 4 + 1] = 255;               // G
        rgbaData[idx * 4 + 2] = 255;               // B
        rgbaData[idx * 4 + 3] = atlasBitmap[idx];  // A
    }

    // Create the texture
    font->m_atlas = std::make_unique<Texture2D>();
    TextureSpec spec;
    spec.width = static_cast<u32>(atlasWidth);
    spec.height = static_cast<u32>(atlasHeight);
    spec.format = TextureFormat::RGBA8;
    spec.minFilter = TextureFilter::Linear;
    spec.magFilter = TextureFilter::Linear;
    spec.generateMipmaps = false;

    auto createResult = font->m_atlas->create(spec, rgbaData.data());
    if (!createResult) {
        return unexpected<String>("Failed to create font atlas texture: " + createResult.error());
    }

    spdlog::debug("Loaded font '{}' with {} glyphs ({}x{} atlas)", path.filename().string(),
                  charCount, atlasWidth, atlasHeight);

    return font;
}

const Glyph& Font::getGlyph(char c) const {
    auto it = m_glyphs.find(c);
    if (it != m_glyphs.end()) {
        return it->second;
    }
    return m_defaultGlyph;
}

}  // namespace limbo
