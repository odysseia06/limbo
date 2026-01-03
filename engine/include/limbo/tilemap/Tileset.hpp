#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/render/common/Texture.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

namespace limbo {

/**
 * TileFlags - Properties for individual tiles
 */
enum class TileFlags : u32 {
    None = 0,
    Solid = 1 << 0,      // Blocks movement/collision
    Platform = 1 << 1,   // One-way platform (solid from above)
    Ladder = 1 << 2,     // Climbable
    Water = 1 << 3,      // Liquid/swimmable
    Hazard = 1 << 4,     // Damages player
    Breakable = 1 << 5,  // Can be destroyed
    Animated = 1 << 6,   // Has animation frames
};

inline TileFlags operator|(TileFlags a, TileFlags b) {
    return static_cast<TileFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline TileFlags operator&(TileFlags a, TileFlags b) {
    return static_cast<TileFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool hasFlag(TileFlags flags, TileFlags flag) {
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

/**
 * TileDefinition - Defines properties of a tile type
 */
struct TileDefinition {
    u32 id = 0;             // Tile ID (index in tileset)
    glm::vec2 uvMin{0.0f};  // UV coordinates in texture
    glm::vec2 uvMax{1.0f};
    TileFlags flags = TileFlags::None;
    String name;  // Optional name for editor

    // Animation (if Animated flag is set)
    std::vector<u32> animationFrames;  // Tile IDs for animation
    f32 animationSpeed = 1.0f;         // Frames per second
};

/**
 * Tileset - A texture atlas containing tiles with metadata
 *
 * Manages a grid of tiles from a single texture, with support for
 * tile properties, collision flags, and animations.
 */
class LIMBO_API Tileset {
public:
    Tileset() = default;
    ~Tileset() = default;

    // Non-copyable, movable
    Tileset(const Tileset&) = delete;
    Tileset& operator=(const Tileset&) = delete;
    Tileset(Tileset&&) = default;
    Tileset& operator=(Tileset&&) = default;

    /**
     * Create tileset from a texture with uniform grid
     * @param texture The tileset texture
     * @param tileWidth Width of each tile in pixels
     * @param tileHeight Height of each tile in pixels
     * @param margin Margin around the texture edge in pixels
     * @param spacing Spacing between tiles in pixels
     */
    void create(Texture2D* texture, u32 tileWidth, u32 tileHeight, u32 margin = 0, u32 spacing = 0);

    /**
     * Get tile definition by ID
     */
    [[nodiscard]] const TileDefinition* getTile(u32 id) const;
    [[nodiscard]] TileDefinition* getTile(u32 id);

    /**
     * Set tile flags
     */
    void setTileFlags(u32 id, TileFlags flags);

    /**
     * Set tile name
     */
    void setTileName(u32 id, const String& name);

    /**
     * Setup tile animation
     */
    void setTileAnimation(u32 id, const std::vector<u32>& frames, f32 speed);

    /**
     * Get UV coordinates for a tile
     */
    [[nodiscard]] glm::vec4
    getTileUV(u32 id) const;  // Returns (uvMin.x, uvMin.y, uvMax.x, uvMax.y)

    // Getters
    [[nodiscard]] Texture2D* getTexture() const { return m_texture; }
    [[nodiscard]] u32 getTileWidth() const { return m_tileWidth; }
    [[nodiscard]] u32 getTileHeight() const { return m_tileHeight; }
    [[nodiscard]] u32 getTileCount() const { return static_cast<u32>(m_tiles.size()); }
    [[nodiscard]] u32 getColumns() const { return m_columns; }
    [[nodiscard]] u32 getRows() const { return m_rows; }

    /**
     * Check if a tile ID is valid
     */
    [[nodiscard]] bool isValidTile(u32 id) const { return id < m_tiles.size(); }

private:
    Texture2D* m_texture = nullptr;
    u32 m_tileWidth = 0;
    u32 m_tileHeight = 0;
    u32 m_columns = 0;
    u32 m_rows = 0;
    u32 m_margin = 0;
    u32 m_spacing = 0;

    std::vector<TileDefinition> m_tiles;
};

}  // namespace limbo
