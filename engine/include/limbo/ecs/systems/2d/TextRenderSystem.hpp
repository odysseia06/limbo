#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"

#include <vector>

namespace limbo {

// Forward declarations
class OrthographicCamera;
class AssetManager;

/**
 * TextRenderSystem - Renders all entities with TextRendererComponent
 *
 * This system uses the TextRenderer to draw text for all entities
 * with a TextRendererComponent. It should run during the render phase.
 *
 * Optimization: Uses a dirty flag pattern to cache sorted entity order.
 * Only re-sorts when text entities are added, removed, or sorting properties change.
 *
 * Required components:
 * - TransformComponent: Position, rotation, scale
 * - TextRendererComponent: Text, font, color, sorting
 */
class LIMBO_API TextRenderSystem : public System {
public:
    TextRenderSystem() = default;
    ~TextRenderSystem() override = default;

    /**
     * Set the camera to use for rendering
     * Must be called before update() each frame
     */
    void setCamera(const OrthographicCamera* camera) { m_camera = camera; }

    /**
     * Set the asset manager for loading fonts
     * Must be set before update() is called
     */
    void setAssetManager(AssetManager* assetManager) { m_assetManager = assetManager; }

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
    AssetManager* m_assetManager = nullptr;

    // Cached sorted entity list
    std::vector<World::EntityId> m_sortedEntities;
    bool m_sortDirty = true;
};

}  // namespace limbo
