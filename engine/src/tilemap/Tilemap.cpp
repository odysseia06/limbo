#include "limbo/tilemap/Tilemap.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace limbo {

void Tilemap::create(u32 width, u32 height, f32 tileWidth, f32 tileHeight) {
    m_width = width;
    m_height = height;
    m_tileWidth = tileWidth;
    m_tileHeight = tileHeight;
    m_layers.clear();
    
    spdlog::debug("Tilemap created: {}x{} tiles, {}x{} units per tile",
                  width, height, tileWidth, tileHeight);
}

void Tilemap::clear() {
    for (auto& layer : m_layers) {
        std::fill(layer.tiles.begin(), layer.tiles.end(), TILE_EMPTY);
    }
}

u32 Tilemap::addLayer(const String& name, i32 zOrder) {
    TilemapLayer layer;
    layer.name = name;
    layer.zOrder = zOrder;
    layer.tiles.resize(static_cast<size_t>(m_width) * m_height, TILE_EMPTY);
    
    m_layers.push_back(std::move(layer));
    
    // Sort layers by zOrder
    std::stable_sort(m_layers.begin(), m_layers.end(),
        [](const TilemapLayer& a, const TilemapLayer& b) {
            return a.zOrder < b.zOrder;
        });
    
    // Find the index of the layer we just added
    for (u32 i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i].name == name) {
            spdlog::debug("Added tilemap layer '{}' at index {} (zOrder: {})", 
                          name, i, zOrder);
            return i;
        }
    }
    
    return static_cast<u32>(m_layers.size() - 1);
}

TilemapLayer* Tilemap::getLayer(u32 index) {
    if (index >= m_layers.size()) return nullptr;
    return &m_layers[index];
}

const TilemapLayer* Tilemap::getLayer(u32 index) const {
    if (index >= m_layers.size()) return nullptr;
    return &m_layers[index];
}

TilemapLayer* Tilemap::getLayerByName(const String& name) {
    for (auto& layer : m_layers) {
        if (layer.name == name) return &layer;
    }
    return nullptr;
}

const TilemapLayer* Tilemap::getLayerByName(const String& name) const {
    for (const auto& layer : m_layers) {
        if (layer.name == name) return &layer;
    }
    return nullptr;
}

u32 Tilemap::getTile(u32 layer, u32 x, u32 y) const {
    if (layer >= m_layers.size() || x >= m_width || y >= m_height) {
        return TILE_EMPTY;
    }
    return m_layers[layer].tiles[tileIndex(x, y)];
}

void Tilemap::setTile(u32 layer, u32 x, u32 y, u32 tileId) {
    if (layer >= m_layers.size() || x >= m_width || y >= m_height) {
        return;
    }
    m_layers[layer].tiles[tileIndex(x, y)] = tileId;
}

void Tilemap::fillRect(u32 layer, u32 x, u32 y, u32 width, u32 height, u32 tileId) {
    if (layer >= m_layers.size()) return;
    
    u32 endX = std::min(x + width, m_width);
    u32 endY = std::min(y + height, m_height);
    
    for (u32 ty = y; ty < endY; ++ty) {
        for (u32 tx = x; tx < endX; ++tx) {
            m_layers[layer].tiles[tileIndex(tx, ty)] = tileId;
        }
    }
}

void Tilemap::fillLayer(u32 layer, u32 tileId) {
    if (layer >= m_layers.size()) return;
    std::fill(m_layers[layer].tiles.begin(), m_layers[layer].tiles.end(), tileId);
}

glm::ivec2 Tilemap::worldToTile(const glm::vec2& worldPos) const {
    return glm::ivec2(
        static_cast<i32>(std::floor(worldPos.x / m_tileWidth)),
        static_cast<i32>(std::floor(worldPos.y / m_tileHeight))
    );
}

glm::vec2 Tilemap::tileToWorld(i32 x, i32 y) const {
    return glm::vec2(
        (static_cast<f32>(x) + 0.5f) * m_tileWidth,
        (static_cast<f32>(y) + 0.5f) * m_tileHeight
    );
}

glm::vec4 Tilemap::getTileBounds(i32 x, i32 y) const {
    f32 minX = static_cast<f32>(x) * m_tileWidth;
    f32 minY = static_cast<f32>(y) * m_tileHeight;
    return glm::vec4(minX, minY, minX + m_tileWidth, minY + m_tileHeight);
}

bool Tilemap::isValidTile(i32 x, i32 y) const {
    return x >= 0 && y >= 0 && 
           static_cast<u32>(x) < m_width && 
           static_cast<u32>(y) < m_height;
}

bool Tilemap::hasTileFlags(i32 x, i32 y, TileFlags flags) const {
    if (!isValidTile(x, y) || !m_tileset) {
        return false;
    }
    
    // Check all layers for tiles with the specified flags
    for (const auto& layer : m_layers) {
        u32 tileId = layer.tiles[tileIndex(static_cast<u32>(x), static_cast<u32>(y))];
        if (tileId == TILE_EMPTY) continue;
        
        const TileDefinition* tile = m_tileset->getTile(tileId);
        if (tile && hasFlag(tile->flags, flags)) {
            return true;
        }
    }
    
    return false;
}

bool Tilemap::isSolidAt(const glm::vec2& worldPos) const {
    glm::ivec2 tilePos = worldToTile(worldPos);
    return hasTileFlags(tilePos.x, tilePos.y, TileFlags::Solid);
}

bool Tilemap::checkCollision(const glm::vec2& min, const glm::vec2& max) const {
    glm::ivec2 tileMin = worldToTile(min);
    glm::ivec2 tileMax = worldToTile(max);
    
    for (i32 y = tileMin.y; y <= tileMax.y; ++y) {
        for (i32 x = tileMin.x; x <= tileMax.x; ++x) {
            if (hasTileFlags(x, y, TileFlags::Solid)) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<glm::ivec2> Tilemap::getSolidTilesInRect(const glm::vec2& min, const glm::vec2& max) const {
    std::vector<glm::ivec2> result;
    
    glm::ivec2 tileMin = worldToTile(min);
    glm::ivec2 tileMax = worldToTile(max);
    
    for (i32 y = tileMin.y; y <= tileMax.y; ++y) {
        for (i32 x = tileMin.x; x <= tileMax.x; ++x) {
            if (hasTileFlags(x, y, TileFlags::Solid)) {
                result.emplace_back(x, y);
            }
        }
    }
    
    return result;
}

} // namespace limbo
