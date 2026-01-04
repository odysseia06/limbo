#include "limbo/tilemap/TilemapRenderer.hpp"
#include "limbo/tilemap/TilemapComponent.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/render/2d/Renderer2D.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

void TilemapRenderer::render(const Tilemap& tilemap, const glm::vec3& position,
                             const OrthographicCamera* camera) {
    for (u32 i = 0; i < tilemap.getLayerCount(); ++i) {
        renderLayer(tilemap, i, position, camera);
    }
}

void TilemapRenderer::renderLayer(const Tilemap& tilemap, u32 layerIndex, const glm::vec3& position,
                                  const OrthographicCamera* camera) {
    const TilemapLayer* layer = tilemap.getLayer(layerIndex);
    if (!layer || !layer->visible) {
        return;
    }

    const Tileset* tileset = tilemap.getTileset();
    if (!tileset || !tileset->getTexture()) {
        return;
    }

    u32 const mapWidth = tilemap.getWidth();
    u32 const mapHeight = tilemap.getHeight();
    f32 const tileWidth = tilemap.getTileWidth();
    f32 const tileHeight = tilemap.getTileHeight();

    // Calculate visible tile range if camera provided
    u32 startX = 0, startY = 0;
    u32 endX = mapWidth, endY = mapHeight;

    if (camera) {
        // Get camera bounds in world space
        glm::vec3 const camPos = camera->getPosition();
        f32 const halfWidth = (camera->getRight() - camera->getLeft()) * 0.5f;
        f32 const halfHeight = (camera->getTop() - camera->getBottom()) * 0.5f;

        glm::vec2 const camMin(camPos.x - halfWidth - position.x,
                               camPos.y - halfHeight - position.y);
        glm::vec2 const camMax(camPos.x + halfWidth - position.x,
                               camPos.y + halfHeight - position.y);

        // Convert to tile coordinates with margin
        glm::ivec2 const tileMin = tilemap.worldToTile(camMin);
        glm::ivec2 const tileMax = tilemap.worldToTile(camMax);

        // Clamp to valid range (avoid negative values before casting to u32)
        i32 const sX = std::max(0, tileMin.x - 1);
        i32 const sY = std::max(0, tileMin.y - 1);
        i32 const eX = std::max(0, std::min(static_cast<i32>(mapWidth), tileMax.x + 2));
        i32 const eY = std::max(0, std::min(static_cast<i32>(mapHeight), tileMax.y + 2));

        startX = static_cast<u32>(sX);
        startY = static_cast<u32>(sY);
        endX = static_cast<u32>(eX);
        endY = static_cast<u32>(eY);
    }

    // Render visible tiles
    for (u32 y = startY; y < endY; ++y) {
        for (u32 x = startX; x < endX; ++x) {
            u32 const tileId = tilemap.getTile(layerIndex, x, y);
            if (tileId == TILE_EMPTY) {
                continue;
            }

            // Calculate tile position
            glm::vec3 const tilePos(position.x + (static_cast<f32>(x) + 0.5f) * tileWidth,
                                    position.y + (static_cast<f32>(y) + 0.5f) * tileHeight,
                                    position.z + static_cast<f32>(layer->zOrder) * 0.001f);

            renderTile(tilemap, tileId, tilePos, layer->opacity);
        }
    }
}

void TilemapRenderer::renderTile(const Tilemap& tilemap, u32 tileId, const glm::vec3& position,
                                 f32 opacity) {
    const Tileset* tileset = tilemap.getTileset();
    if (!tileset) {
        return;
    }

    const TileDefinition* tile = tileset->getTile(tileId);
    if (!tile) {
        return;
    }

    glm::vec2 const size(tilemap.getTileWidth(), tilemap.getTileHeight());
    glm::vec4 const color(1.0f, 1.0f, 1.0f, opacity);

    Renderer2D::drawQuad(position, size, *tileset->getTexture(), tile->uvMin, tile->uvMax, color);
}

// ============================================================================
// TilemapRenderSystem
// ============================================================================

void TilemapRenderSystem::onAttach(World& world) {
    (void)world;
    spdlog::debug("TilemapRenderSystem attached");
}

void TilemapRenderSystem::update(World& world, f32 deltaTime) {
    (void)world;
    (void)deltaTime;
    // Animation updates could go here
}

void TilemapRenderSystem::onDetach(World& world) {
    (void)world;
    spdlog::debug("TilemapRenderSystem detached");
}

void TilemapRenderSystem::render(World& world) {
    world.each<TransformComponent, TilemapComponent>(
        [this](World::EntityId, TransformComponent& transform, TilemapComponent& tilemapComp) {
            if (!tilemapComp.visible || !tilemapComp.tilemap) {
                return;
            }

            glm::vec3 pos = transform.position;
            pos.x += tilemapComp.offset.x;
            pos.y += tilemapComp.offset.y;

            TilemapRenderer::render(*tilemapComp.tilemap, pos, m_camera);
        });
}

}  // namespace limbo
