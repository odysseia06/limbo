#include "limbo/assets/TextureAsset.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

bool TextureAsset::load() {
    m_texture = make_unique<Texture2D>();

    auto result = m_texture->loadFromFile(getPath());
    if (!result) {
        setError(result.error());
        spdlog::error("Failed to load texture '{}': {}", getPath().string(), result.error());
        m_texture.reset();
        return false;
    }

    spdlog::debug("Loaded texture: {} ({}x{})", getPath().string(), m_texture->getWidth(),
                  m_texture->getHeight());
    return true;
}

void TextureAsset::unload() {
    m_texture.reset();
}

}  // namespace limbo
