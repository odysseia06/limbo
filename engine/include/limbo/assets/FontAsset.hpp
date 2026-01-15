#pragma once

#include "limbo/assets/Asset.hpp"
#include "limbo/render/2d/Font.hpp"

namespace limbo {

/**
 * FontAsset - Managed font asset
 *
 * Wraps a Font and provides asset management features like
 * loading from disk and hot-reloading.
 *
 * Font size can be configured before loading via setFontSize().
 * Default size is 32 pixels.
 */
class LIMBO_API FontAsset : public Asset {
public:
    FontAsset() = default;
    ~FontAsset() override = default;

    [[nodiscard]] AssetType getType() const override { return AssetType::Font; }

    /**
     * Get the underlying font
     */
    [[nodiscard]] Font* getFont() { return m_font.get(); }
    [[nodiscard]] const Font* getFont() const { return m_font.get(); }

    /**
     * Get the font size in pixels
     */
    [[nodiscard]] f32 getFontSize() const { return m_fontSize; }

    /**
     * Set the font size (must be called before loading)
     * @param size Font size in pixels
     */
    void setFontSize(f32 size) { m_fontSize = size; }

    /**
     * Get font metrics
     */
    [[nodiscard]] f32 getLineHeight() const { return m_font ? m_font->getLineHeight() : 0.0f; }
    [[nodiscard]] f32 getAscent() const { return m_font ? m_font->getAscent() : 0.0f; }
    [[nodiscard]] f32 getDescent() const { return m_font ? m_font->getDescent() : 0.0f; }

protected:
    friend class AssetManager;

    bool load() override;
    void unload() override;

private:
    Unique<Font> m_font;
    f32 m_fontSize = 32.0f;
};

}  // namespace limbo
