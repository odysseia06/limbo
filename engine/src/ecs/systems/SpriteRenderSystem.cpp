#include "limbo/ecs/systems/SpriteRenderSystem.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/render/Renderer2D.hpp"
#include "limbo/render/Camera.hpp"

#include <algorithm>
#include <vector>

namespace limbo {

void SpriteRenderSystem::onAttach(World& /*world*/) {
    // Renderer2D should be initialized by the application
}

void SpriteRenderSystem::onDetach(World& /*world*/) {
    // Renderer2D shutdown is handled by the application
}

void SpriteRenderSystem::update(World& world, f32 /*deltaTime*/) {
    if (!m_camera) {
        return;
    }

    // Collect all renderable entities for sorting
    struct RenderableEntity {
        World::EntityId entity;
        const TransformComponent* transform;
        const SpriteRendererComponent* sprite;
    };

    std::vector<RenderableEntity> renderables;

    auto view = world.view<TransformComponent, SpriteRendererComponent>();
    for (auto entity : view) {
        const auto& transform = view.get<TransformComponent>(entity);
        const auto& sprite = view.get<SpriteRendererComponent>(entity);

        renderables.push_back({entity, &transform, &sprite});
    }

    // Sort by sorting order (lower values render first / behind)
    std::sort(renderables.begin(), renderables.end(),
              [](const RenderableEntity& a, const RenderableEntity& b) {
                  return a.sprite->sortingOrder < b.sprite->sortingOrder;
              });

    // Begin rendering
    Renderer2D::beginScene(*m_camera);

    // Render all sprites
    for (const auto& renderable : renderables) {
        const auto& transform = *renderable.transform;
        const auto& sprite = *renderable.sprite;

        // Build transform matrix
        glm::mat4 transformMatrix = transform.getMatrix();

        // Draw the quad with the sprite's color
        // TODO: Support textured sprites via AssetId lookup
        Renderer2D::drawQuad(transformMatrix, sprite.color);
    }

    Renderer2D::endScene();
}

}  // namespace limbo
