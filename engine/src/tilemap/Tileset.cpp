#include "limbo/tilemap/Tileset.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo {

void Tileset::create(Texture2D* texture, u32 tileWidth, u32 tileHeight, u32 margin, u32 spacing) {
    if (!texture) {
        LIMBO_LOG_RENDER_ERROR("Tileset::create: null texture");
        return;
    }

    m_texture = texture;
    m_tileWidth = tileWidth;
    m_tileHeight = tileHeight;
    m_margin = margin;
    m_spacing = spacing;

    // Calculate grid dimensions
    u32 const texWidth = texture->getWidth();
    u32 const texHeight = texture->getHeight();

    u32 const usableWidth = texWidth - 2 * margin;
    u32 const usableHeight = texHeight - 2 * margin;

    m_columns = (usableWidth + spacing) / (tileWidth + spacing);
    m_rows = (usableHeight + spacing) / (tileHeight + spacing);

    u32 tileCount = m_columns * m_rows;
    m_tiles.resize(tileCount);

    // Pre-calculate UV coordinates for each tile
    f32 const texWidthF = static_cast<f32>(texWidth);
    f32 const texHeightF = static_cast<f32>(texHeight);

    for (u32 i = 0; i < tileCount; ++i) {
        u32 const col = i % m_columns;
        u32 const row = i / m_columns;

        // Calculate pixel position
        u32 const px = margin + col * (tileWidth + spacing);
        u32 const py = margin + row * (tileHeight + spacing);

        // Convert to UV coordinates (flip Y for OpenGL)
        m_tiles[i].id = i;
        m_tiles[i].uvMin.x = static_cast<f32>(px) / texWidthF;
        m_tiles[i].uvMin.y = static_cast<f32>(py) / texHeightF;
        m_tiles[i].uvMax.x = static_cast<f32>(px + tileWidth) / texWidthF;
        m_tiles[i].uvMax.y = static_cast<f32>(py + tileHeight) / texHeightF;
        m_tiles[i].flags = TileFlags::None;
    }

    LIMBO_LOG_RENDER_DEBUG("Tileset created: {}x{} tiles ({}x{} px each), {} total", m_columns,
                           m_rows, tileWidth, tileHeight, tileCount);
}

const TileDefinition* Tileset::getTile(u32 id) const {
    if (id >= m_tiles.size()) {
        return nullptr;
    }
    return &m_tiles[id];
}

TileDefinition* Tileset::getTile(u32 id) {
    if (id >= m_tiles.size()) {
        return nullptr;
    }
    return &m_tiles[id];
}

void Tileset::setTileFlags(u32 id, TileFlags flags) {
    if (auto* tile = getTile(id)) {
        tile->flags = flags;
    }
}

void Tileset::setTileName(u32 id, const String& name) {
    if (auto* tile = getTile(id)) {
        tile->name = name;
    }
}

void Tileset::setTileAnimation(u32 id, const std::vector<u32>& frames, f32 speed) {
    if (auto* tile = getTile(id)) {
        tile->animationFrames = frames;
        tile->animationSpeed = speed;
        tile->flags = tile->flags | TileFlags::Animated;
    }
}

glm::vec4 Tileset::getTileUV(u32 id) const {
    if (id >= m_tiles.size()) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    const auto& tile = m_tiles[id];
    return glm::vec4(tile.uvMin.x, tile.uvMin.y, tile.uvMax.x, tile.uvMax.y);
}

}  // namespace limbo
