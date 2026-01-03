#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/ecs/System.hpp"

namespace limbo {

// Forward declarations
class OrthographicCamera;

/**
 * SpriteRenderSystem - Renders all entities with SpriteRendererComponent
 *
 * This system uses the batched Renderer2D to efficiently draw all sprites
 * in the world. It should run during the render phase.
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

    void onAttach(World& world) override;
    void onDetach(World& world) override;
    void update(World& world, f32 deltaTime) override;

private:
    const OrthographicCamera* m_camera = nullptr;
};

}  // namespace limbo
