#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"

#include <vector>

namespace limbo {

// Forward declarations
class OrthographicCamera;

/**
 * SpriteRenderSystem - Renders all entities with SpriteRendererComponent
 *
 * This system uses the batched Renderer2D to efficiently draw all sprites
 * in the world. It should run during the render phase.
 *
 * Optimization: Uses a dirty flag pattern to cache sorted entity order.
 * Only re-sorts when sprites are added, removed, or sorting properties change.
 *
 * Required components:
 * - TransformComponent: Position, rotation, scale
 * - SpriteRendererComponent: Color, texture, sorting
 */
class LIMBO_API SpriteRenderSystem : public System {
public:
    SpriteRenderSystem() = default;
    ~SpriteRenderSystem() override = default;

    /**
     * Set the camera to use for rendering
     * Must be called before update() each frame
     */
    void setCamera(const OrthographicCamera* camera) { m_camera = camera; }

    /**
     * Mark the sort order as dirty
     * Call this when sorting properties change
     */
    void markDirty() { m_sortDirty = true; }

    void onAttach(World& world) override;
    void onDetach(World& world) override;
    void update(World& world, f32 deltaTime) override;

private:
    void rebuildSortedList(World& world);

    const OrthographicCamera* m_camera = nullptr;

    // Cached sorted entity list
    std::vector<World::EntityId> m_sortedEntities;
    bool m_sortDirty = true;
};

}  // namespace limbo
