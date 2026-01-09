#pragma once

#include "limbo/assets/Asset.hpp"
#include "limbo/render/2d/SpriteAtlas.hpp"

namespace limbo {

/**
 * SpriteAtlasAsset - Managed sprite atlas asset
 *
 * Wraps a SpriteAtlas and provides asset management features like
 * loading from disk and hot-reloading.
 *
 * Loads from .atlas JSON metadata files which reference a texture file.
 */
class LIMBO_API SpriteAtlasAsset : public Asset {
public:
    SpriteAtlasAsset() = default;
    ~SpriteAtlasAsset() override = default;

    [[nodiscard]] AssetType getType() const override { return AssetType::SpriteAtlas; }

    /**
     * Get the underlying sprite atlas
     */
    [[nodiscard]] SpriteAtlas* getAtlas() { return m_atlas.get(); }
    [[nodiscard]] const SpriteAtlas* getAtlas() const { return m_atlas.get(); }

    /**
     * Get the atlas texture
     */
    [[nodiscard]] Texture2D* getTexture() { return m_atlas ? m_atlas->getTexture() : nullptr; }
    [[nodiscard]] const Texture2D* getTexture() const {
        return m_atlas ? m_atlas->getTexture() : nullptr;
    }

    /**
     * Get atlas dimensions
     */
    [[nodiscard]] u32 getWidth() const { return m_atlas ? m_atlas->getWidth() : 0; }
    [[nodiscard]] u32 getHeight() const { return m_atlas ? m_atlas->getHeight() : 0; }

    /**
     * Get a sprite region by name
     */
    [[nodiscard]] const SpriteRegion* getRegion(const String& name) const {
        return m_atlas ? m_atlas->getRegion(name) : nullptr;
    }

    /**
     * Get region count
     */
    [[nodiscard]] usize getRegionCount() const { return m_atlas ? m_atlas->getRegionCount() : 0; }

    /**
     * Bind the atlas texture to a slot
     */
    void bind(u32 slot = 0) const {
        if (m_atlas && m_atlas->getTexture()) {
            m_atlas->getTexture()->bind(slot);
        }
    }

    /**
     * Get dependencies for hot-reloading
     * Includes both the .atlas file and the texture file
     */
    [[nodiscard]] std::vector<std::filesystem::path> getDependencies() const override;

protected:
    friend class AssetManager;

    bool load() override;
    void unload() override;

private:
    Unique<SpriteAtlas> m_atlas;
    std::filesystem::path m_texturePath;  // Full path to texture for hot-reload tracking
};

}  // namespace limbo
