#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/render/2d/SpriteAtlas.hpp"

#include <filesystem>
#include <vector>

namespace limbo {

/**
 * Configuration for atlas building
 */
struct LIMBO_API AtlasBuildConfig {
    /// Maximum atlas dimensions
    u32 maxWidth = 4096;
    u32 maxHeight = 4096;

    /// Padding between sprites (in pixels)
    u32 padding = 2;

    /// Whether to allow 90-degree rotation for better packing
    bool allowRotation = false;

    /// Whether to generate mipmaps for the atlas texture
    bool generateMipmaps = true;

    /// Whether to use power-of-two dimensions
    bool powerOfTwo = true;

    /// Whether to trim transparent pixels from sprites
    bool trimTransparent = false;

    /// Background color for empty atlas space (RGBA)
    u32 backgroundColor = 0x00000000;
};

/**
 * Input sprite for atlas building
 */
struct LIMBO_API AtlasInputSprite {
    /// Unique name for this sprite
    String name;

    /// Source file path
    std::filesystem::path path;

    /// Custom pivot point (optional, default is center)
    glm::vec2 pivot{0.5f, 0.5f};

    /// Image data (loaded by builder)
    std::vector<u8> pixels;
    u32 width = 0;
    u32 height = 0;
    u32 channels = 0;
};

/**
 * Result of atlas building
 */
struct LIMBO_API AtlasBuildResult {
    bool success = false;
    String error;

    /// The built atlas (moved on success)
    Unique<SpriteAtlas> atlas;

    /// Sprites that couldn't fit
    std::vector<String> overflow;

    /// Build statistics
    u32 totalSprites = 0;
    u32 packedSprites = 0;
    f32 packingEfficiency = 0.0f;  // 0-1, ratio of used to total area
};

/**
 * Rectangle for packing algorithm
 */
struct PackRect {
    u32 x = 0;
    u32 y = 0;
    u32 width = 0;
    u32 height = 0;
    i32 spriteIndex = -1;  // Index into input sprites, -1 if free space
    bool rotated = false;
};

/**
 * SpriteAtlasBuilder - Packs multiple sprites into a texture atlas
 *
 * Uses the MaxRects bin packing algorithm for efficient packing.
 *
 * Usage:
 *   SpriteAtlasBuilder builder;
 *   builder.addSprite("player", "sprites/player.png");
 *   builder.addSprite("enemy", "sprites/enemy.png");
 *   AtlasBuildResult result = builder.build(config);
 *   if (result.success) {
 *       // Use result.atlas
 *   }
 */
class LIMBO_API SpriteAtlasBuilder {
public:
    SpriteAtlasBuilder() = default;
    ~SpriteAtlasBuilder() = default;

    // Non-copyable
    SpriteAtlasBuilder(const SpriteAtlasBuilder&) = delete;
    SpriteAtlasBuilder& operator=(const SpriteAtlasBuilder&) = delete;

    /**
     * Add a sprite to the atlas
     * @param name Unique name for the sprite
     * @param path Path to the image file
     * @param pivot Optional pivot point (default center)
     */
    void addSprite(const String& name, const std::filesystem::path& path,
                   const glm::vec2& pivot = glm::vec2(0.5f));

    /**
     * Add all images from a directory
     * @param directory Directory to scan
     * @param recursive Whether to scan subdirectories
     * @param extensions File extensions to include (e.g., {".png", ".jpg"})
     */
    void addDirectory(const std::filesystem::path& directory, bool recursive = false,
                      const std::vector<String>& extensions = {".png", ".jpg", ".jpeg", ".bmp"});

    /**
     * Clear all added sprites
     */
    void clear();

    /**
     * Get the number of sprites added
     */
    [[nodiscard]] usize getSpriteCount() const { return m_sprites.size(); }

    /**
     * Build the atlas
     * @param config Build configuration
     * @return Build result with atlas or error
     */
    [[nodiscard]] AtlasBuildResult build(const AtlasBuildConfig& config = {});

    /**
     * Save the built atlas to disk
     * @param atlas The atlas to save
     * @param atlasPath Path for the .atlas metadata file
     * @param texturePath Path for the texture image
     * @return true on success
     */
    [[nodiscard]] static bool saveAtlas(const SpriteAtlas& atlas,
                                        const std::filesystem::path& atlasPath,
                                        const std::filesystem::path& texturePath);

private:
    std::vector<AtlasInputSprite> m_sprites;

    /**
     * Load image data for all sprites
     */
    [[nodiscard]] bool loadImages();

    /**
     * Calculate optimal atlas size
     */
    [[nodiscard]] glm::uvec2 calculateAtlasSize(const AtlasBuildConfig& config) const;

    /**
     * Round up to next power of two
     */
    [[nodiscard]] static u32 nextPowerOfTwo(u32 value);

    /**
     * Pack sprites using MaxRects algorithm
     * @param atlasWidth Atlas width
     * @param atlasHeight Atlas height
     * @param config Build config
     * @return Packed rectangles (one per sprite, plus free rects)
     */
    [[nodiscard]] std::vector<PackRect> packSprites(u32 atlasWidth, u32 atlasHeight,
                                                    const AtlasBuildConfig& config);

    /**
     * Create the atlas texture from packed sprites
     */
    [[nodiscard]] Unique<Texture2D> createTexture(const std::vector<PackRect>& packed,
                                                  u32 atlasWidth, u32 atlasHeight,
                                                  const AtlasBuildConfig& config);
};

}  // namespace limbo
