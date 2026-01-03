#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/tilemap/Tileset.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

// Special tile value indicating empty/no tile
constexpr u32 TILE_EMPTY = 0xFFFFFFFF;

/**
 * TilemapLayer - A single layer of tiles
 */
struct TilemapLayer {
    String name;
    std::vector<u32> tiles;  // Tile indices (row-major order)
    bool visible = true;
    f32 opacity = 1.0f;
    glm::vec2 parallax{1.0f};  // Parallax scrolling factor
    i32 zOrder = 0;            // Render order
};

/**
 * Tilemap - A 2D grid of tiles organized in layers
 *
 * Supports multiple layers, different tilesets per layer,
 * and efficient querying for collision detection.
 */
class LIMBO_API Tilemap {
public:
    Tilemap() = default;
    ~Tilemap() = default;

    /**
     * Create a tilemap with given dimensions
     * @param width Number of tiles horizontally
     * @param height Number of tiles vertically
     * @param tileWidth Width of each tile in world units
     * @param tileHeight Height of each tile in world units
     */
    void create(u32 width, u32 height, f32 tileWidth, f32 tileHeight);

    /**
     * Clear all tiles and layers
     */
    void clear();

    // ========================================================================
    // Layer Management
    // ========================================================================

    /**
     * Add a new layer
     * @param name Layer name
     * @param zOrder Render order (lower = rendered first/behind)
     * @return Layer index
     */
    u32 addLayer(const String& name, i32 zOrder = 0);

    /**
     * Get layer by index
     */
    [[nodiscard]] TilemapLayer* getLayer(u32 index);
    [[nodiscard]] const TilemapLayer* getLayer(u32 index) const;

    /**
     * Get layer by name
     */
    [[nodiscard]] TilemapLayer* getLayerByName(const String& name);
    [[nodiscard]] const TilemapLayer* getLayerByName(const String& name) const;

    /**
     * Get number of layers
     */
    [[nodiscard]] u32 getLayerCount() const { return static_cast<u32>(m_layers.size()); }

    // ========================================================================
    // Tile Access
    // ========================================================================

    /**
     * Get tile at position in a layer
     * @param layer Layer index
     * @param x Tile X coordinate
     * @param y Tile Y coordinate
     * @return Tile index, or TILE_EMPTY if out of bounds
     */
    [[nodiscard]] u32 getTile(u32 layer, u32 x, u32 y) const;

    /**
     * Set tile at position in a layer
     */
    void setTile(u32 layer, u32 x, u32 y, u32 tileId);

    /**
     * Fill a region with a tile
     */
    void fillRect(u32 layer, u32 x, u32 y, u32 width, u32 height, u32 tileId);

    /**
     * Fill entire layer with a tile
     */
    void fillLayer(u32 layer, u32 tileId);

    // ========================================================================
    // Coordinate Conversion
    // ========================================================================

    /**
     * Convert world position to tile coordinates
     */
    [[nodiscard]] glm::ivec2 worldToTile(const glm::vec2& worldPos) const;

    /**
     * Convert tile coordinates to world position (center of tile)
     */
    [[nodiscard]] glm::vec2 tileToWorld(i32 x, i32 y) const;

    /**
     * Get the bounding box of a tile in world coordinates
     */
    [[nodiscard]] glm::vec4 getTileBounds(i32 x, i32 y) const;  // (minX, minY, maxX, maxY)

    /**
     * Check if tile coordinates are valid
     */
    [[nodiscard]] bool isValidTile(i32 x, i32 y) const;

    // ========================================================================
    // Collision Queries
    // ========================================================================

    /**
     * Check if a tile has specific flags (in any layer with collision)
     */
    [[nodiscard]] bool hasTileFlags(i32 x, i32 y, TileFlags flags) const;

    /**
     * Check if a world position overlaps a solid tile
     */
    [[nodiscard]] bool isSolidAt(const glm::vec2& worldPos) const;

    /**
     * Check if a world rectangle overlaps any solid tiles
     * @return true if collision detected
     */
    [[nodiscard]] bool checkCollision(const glm::vec2& min, const glm::vec2& max) const;

    /**
     * Get all solid tiles overlapping a rectangle
     */
    [[nodiscard]] std::vector<glm::ivec2> getSolidTilesInRect(const glm::vec2& min,
                                                              const glm::vec2& max) const;

    // ========================================================================
    // Tileset
    // ========================================================================

    void setTileset(Tileset* tileset) { m_tileset = tileset; }
    [[nodiscard]] Tileset* getTileset() const { return m_tileset; }

    // ========================================================================
    // Properties
    // ========================================================================

    [[nodiscard]] u32 getWidth() const { return m_width; }
    [[nodiscard]] u32 getHeight() const { return m_height; }
    [[nodiscard]] f32 getTileWidth() const { return m_tileWidth; }
    [[nodiscard]] f32 getTileHeight() const { return m_tileHeight; }
    [[nodiscard]] glm::vec2 getWorldSize() const {
        return glm::vec2(m_width * m_tileWidth, m_height * m_tileHeight);
    }

private:
    [[nodiscard]] u32 tileIndex(u32 x, u32 y) const { return y * m_width + x; }

    u32 m_width = 0;
    u32 m_height = 0;
    f32 m_tileWidth = 1.0f;
    f32 m_tileHeight = 1.0f;

    std::vector<TilemapLayer> m_layers;
    Tileset* m_tileset = nullptr;
};

}  // namespace limbo
