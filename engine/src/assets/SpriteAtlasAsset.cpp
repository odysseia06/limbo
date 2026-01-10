#include "limbo/assets/SpriteAtlasAsset.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

bool SpriteAtlasAsset::load() {
    const auto& atlasPath = getPath();

    if (!std::filesystem::exists(atlasPath)) {
        setError("Atlas file not found: " + atlasPath.string());
        return false;
    }

    // Create atlas and load metadata
    m_atlas = std::make_unique<SpriteAtlas>();
    String textureName = m_atlas->loadMetadata(atlasPath);

    if (textureName.empty()) {
        setError("Failed to load atlas metadata: " + atlasPath.string());
        m_atlas.reset();
        return false;
    }

    // Resolve texture path relative to atlas file
    m_texturePath = atlasPath.parent_path() / textureName;

    if (!std::filesystem::exists(m_texturePath)) {
        setError("Atlas texture not found: " + m_texturePath.string());
        m_atlas.reset();
        return false;
    }

    // Load the texture
    auto texture = std::make_unique<Texture2D>();
    auto result = texture->loadFromFile(m_texturePath);
    if (!result) {
        setError("Failed to load atlas texture: " + result.error());
        m_atlas.reset();
        return false;
    }

    m_atlas->setTexture(std::move(texture));

    LIMBO_LOG_ASSET_DEBUG("SpriteAtlasAsset: Loaded atlas with {} regions from {}",
                          m_atlas->getRegionCount(), atlasPath.string());

    return true;
}

void SpriteAtlasAsset::unload() {
    m_atlas.reset();
    m_texturePath.clear();
}

std::vector<std::filesystem::path> SpriteAtlasAsset::getDependencies() const {
    std::vector<std::filesystem::path> deps;
    deps.push_back(getPath());  // The .atlas file

    if (!m_texturePath.empty()) {
        deps.push_back(m_texturePath);  // The texture file
    }

    return deps;
}

}  // namespace limbo
