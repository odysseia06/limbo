#pragma once

#include "limbo/assets/Asset.hpp"
#include "limbo/render/common/Texture.hpp"

#include <vector>

namespace limbo {

/**
 * TextureAsset - Managed texture asset
 *
 * Wraps a Texture2D and provides asset management features like
 * loading from disk and hot-reloading.
 *
 * Supports async loading: loadIO() decodes the image on a worker thread,
 * uploadGPU() creates the OpenGL texture on the main thread.
 */
class LIMBO_API TextureAsset : public Asset {
public:
    TextureAsset() = default;
    ~TextureAsset() override = default;

    [[nodiscard]] AssetType getType() const override { return AssetType::Texture; }

    /**
     * Get the underlying texture
     */
    [[nodiscard]] Texture2D* getTexture() { return m_texture.get(); }
    [[nodiscard]] const Texture2D* getTexture() const { return m_texture.get(); }

    /**
     * Get texture dimensions
     */
    [[nodiscard]] u32 getWidth() const { return m_texture ? m_texture->getWidth() : 0; }
    [[nodiscard]] u32 getHeight() const { return m_texture ? m_texture->getHeight() : 0; }

    /**
     * Bind the texture to a slot
     */
    void bind(u32 slot = 0) const {
        if (m_texture) {
            m_texture->bind(slot);
        }
    }

protected:
    friend class AssetManager;
    friend class AssetLoader;

    bool load() override;
    bool loadIO() override;
    bool uploadGPU() override;
    [[nodiscard]] bool supportsAsyncLoad() const override { return true; }
    void unload() override;

private:
    Unique<Texture2D> m_texture;

    // Intermediate data for async loading (populated by loadIO, consumed by uploadGPU)
    std::vector<u8> m_pendingData;
    TextureSpec m_pendingSpec;
};

}  // namespace limbo
