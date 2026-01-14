#include "limbo/assets/FontAsset.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

bool FontAsset::load() {
    auto result = Font::loadFromFile(getPath(), m_fontSize);
    if (!result) {
        setError(result.error());
        LIMBO_LOG_ASSET_ERROR("Failed to load font '{}': {}", getPath().string(), result.error());
        return false;
    }

    m_font = std::move(result.value());

    LIMBO_LOG_ASSET_DEBUG("Loaded font: {} (size: {}, line height: {})", getPath().string(),
                          m_fontSize, m_font->getLineHeight());
    return true;
}

void FontAsset::unload() {
    m_font.reset();
}

}  // namespace limbo
