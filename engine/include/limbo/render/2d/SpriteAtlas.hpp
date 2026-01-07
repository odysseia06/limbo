#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/render/common/Texture.hpp"

#include <glm/glm.hpp>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace limbo {

/**
 * Sprite region within an atlas
 */
struct LIMBO_API SpriteRegion {
    /// Name/identifier for this sprite
    String name;

    /// UV coordinates (normalized 0-1)
    glm::vec2 uvMin{0.0f, 0.0f};
    glm::vec2 uvMax{1.0f, 1.0f};

    /// Size in pixels
    u32 width = 0;
    u32 height = 0;

    /// Position in atlas (pixels)
    u32 x = 0;
    u32 y = 0;

    /// Pivot point (normalized, 0.5 = center)
    glm::vec2 pivot{0.5f, 0.5f};

    /// Original source file (for rebuild tracking)
    String sourceFile;

    /// Whether this region is rotated 90 degrees in the atlas
    bool rotated = false;
};

/**
 * SpriteAtlas - A packed texture containing multiple sprites
 *
 * The atlas stores:
 * - A single texture containing all packed sprites
 * - Named regions for each sprite with UV coordinates
 * - Metadata for rebuild tracking and hot-reload
 *
 * Use SpriteAtlasBuilder to create atlases from individual images.
 */
class LIMBO_API SpriteAtlas {
public:
    SpriteAtlas() = default;
    ~SpriteAtlas() = default;

    // Non-copyable, movable
    SpriteAtlas(const SpriteAtlas&) = delete;
    SpriteAtlas& operator=(const SpriteAtlas&) = delete;
    SpriteAtlas(SpriteAtlas&&) = default;
    SpriteAtlas& operator=(SpriteAtlas&&) = default;

    /**
     * Get the atlas texture
     */
    [[nodiscard]] Texture2D* getTexture() { return m_texture.get(); }
    [[nodiscard]] const Texture2D* getTexture() const { return m_texture.get(); }

    /**
     * Set the atlas texture (takes ownership)
     */
    void setTexture(Unique<Texture2D> texture) { m_texture = std::move(texture); }

    /**
     * Get atlas dimensions
     */
    [[nodiscard]] u32 getWidth() const { return m_width; }
    [[nodiscard]] u32 getHeight() const { return m_height; }

    /**
     * Set atlas dimensions
     */
    void setSize(u32 width, u32 height) {
        m_width = width;
        m_height = height;
    }

    // ========================================================================
    // Sprite Region Management
    // ========================================================================

    /**
     * Add a sprite region
     */
    void addRegion(const SpriteRegion& region);

    /**
     * Get a sprite region by name
     * @return Pointer to region, or nullptr if not found
     */
    [[nodiscard]] const SpriteRegion* getRegion(const String& name) const;

    /**
     * Get a sprite region by index
     */
    [[nodiscard]] const SpriteRegion& getRegionByIndex(usize index) const;

    /**
     * Check if a region exists
     */
    [[nodiscard]] bool hasRegion(const String& name) const;

    /**
     * Get all region names
     */
    [[nodiscard]] std::vector<String> getRegionNames() const;

    /**
     * Get total number of regions
     */
    [[nodiscard]] usize getRegionCount() const { return m_regions.size(); }

    /**
     * Get all regions
     */
    [[nodiscard]] const std::vector<SpriteRegion>& getRegions() const { return m_regions; }

    /**
     * Clear all regions
     */
    void clearRegions();

    // ========================================================================
    // Serialization
    // ========================================================================

    /**
     * Save atlas metadata to JSON file
     * @param path Path to save the .atlas file
     * @param texturePath Relative path to the texture file
     */
    [[nodiscard]] bool saveMetadata(const std::filesystem::path& path,
                                    const String& texturePath) const;

    /**
     * Load atlas metadata from JSON file
     * @param path Path to the .atlas file
     * @return Path to the texture file (relative to atlas file)
     */
    [[nodiscard]] String loadMetadata(const std::filesystem::path& path);

    /**
     * Check if atlas is valid (has texture and regions)
     */
    [[nodiscard]] bool isValid() const { return m_texture != nullptr && !m_regions.empty(); }

private:
    Unique<Texture2D> m_texture;
    std::vector<SpriteRegion> m_regions;
    std::unordered_map<String, usize> m_nameToIndex;
    u32 m_width = 0;
    u32 m_height = 0;
};

}  // namespace limbo
