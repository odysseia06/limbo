#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/tilemap/Tilemap.hpp"
#include "limbo/render/common/Camera.hpp"

#include <glm/glm.hpp>

namespace limbo {

/**
 * TilemapRenderer - Renders tilemaps using the batch renderer
 *
 * This is a utility class for rendering tilemaps. It can be used
 * standalone or through the TilemapRenderSystem.
 */
class LIMBO_API TilemapRenderer {
public:
    /**
     * Render a tilemap
     * @param tilemap The tilemap to render
     * @param position World position of the tilemap origin
     * @param camera Optional camera for culling (if null, renders all tiles)
     */
    static void render(const Tilemap& tilemap, const glm::vec3& position,
                       const OrthographicCamera* camera = nullptr);

    /**
     * Render a single layer of a tilemap
     */
    static void renderLayer(const Tilemap& tilemap, u32 layerIndex, const glm::vec3& position,
                            const OrthographicCamera* camera = nullptr);

private:
    static void renderTile(const Tilemap& tilemap, u32 tileId, const glm::vec3& position,
                           f32 opacity);
};

/**
 * TilemapRenderSystem - ECS system for rendering TilemapComponents
 */
class LIMBO_API TilemapRenderSystem : public System {
public:
    TilemapRenderSystem() = default;

    void setCamera(const OrthographicCamera* camera) { m_camera = camera; }

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;

    /**
     * Render all tilemaps (call during render phase)
     */
    void render(World& world);

private:
    const OrthographicCamera* m_camera = nullptr;
};

}  // namespace limbo
