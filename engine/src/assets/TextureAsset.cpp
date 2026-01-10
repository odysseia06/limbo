#include "limbo/assets/TextureAsset.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

bool TextureAsset::load() {
    m_texture = make_unique<Texture2D>();

    auto result = m_texture->loadFromFile(getPath());
    if (!result) {
        setError(result.error());
        LIMBO_LOG_ASSET_ERROR("Failed to load texture '{}': {}", getPath().string(),
                              result.error());
        m_texture.reset();
        return false;
    }

    LIMBO_LOG_ASSET_DEBUG("Loaded texture: {} ({}x{})", getPath().string(), m_texture->getWidth(),
                          m_texture->getHeight());
    return true;
}

void TextureAsset::unload() {
    m_texture.reset();
}

}  // namespace limbo
