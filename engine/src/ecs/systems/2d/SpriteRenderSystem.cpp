#include "limbo/ecs/systems/2d/SpriteRenderSystem.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/common/Camera.hpp"

#include <algorithm>

namespace limbo {

void SpriteRenderSystem::onAttach(World& world) {
    // Subscribe to component add/remove via EnTT signals to mark dirty
    auto& registry = world.registry();

    // When SpriteRendererComponent is added or removed, we need to re-sort
    registry.on_construct<SpriteRendererComponent>().connect<&SpriteRenderSystem::markDirty>(this);
    registry.on_destroy<SpriteRendererComponent>().connect<&SpriteRenderSystem::markDirty>(this);

    // Also watch TransformComponent since entities need both
    registry.on_construct<TransformComponent>().connect<&SpriteRenderSystem::markDirty>(this);
    registry.on_destroy<TransformComponent>().connect<&SpriteRenderSystem::markDirty>(this);

    m_sortDirty = true;
}

void SpriteRenderSystem::onDetach(World& world) {
    // Disconnect signals
    auto& registry = world.registry();

    registry.on_construct<SpriteRendererComponent>().disconnect<&SpriteRenderSystem::markDirty>(
        this);
    registry.on_destroy<SpriteRendererComponent>().disconnect<&SpriteRenderSystem::markDirty>(this);
    registry.on_construct<TransformComponent>().disconnect<&SpriteRenderSystem::markDirty>(this);
    registry.on_destroy<TransformComponent>().disconnect<&SpriteRenderSystem::markDirty>(this);

    m_sortedEntities.clear();
}

void SpriteRenderSystem::rebuildSortedList(World& world) {
    m_sortedEntities.clear();

    auto view = world.view<TransformComponent, SpriteRendererComponent>();

    // Reserve capacity
    m_sortedEntities.reserve(128);

    for (auto entity : view) {
        m_sortedEntities.push_back(entity);
    }

    // Sort by layer first, then by order within layer
    std::sort(m_sortedEntities.begin(), m_sortedEntities.end(),
              [&view](World::EntityId a, World::EntityId b) {
                  const auto& spriteA = view.get<SpriteRendererComponent>(a);
                  const auto& spriteB = view.get<SpriteRendererComponent>(b);

                  if (spriteA.sortingLayer != spriteB.sortingLayer) {
                      return spriteA.sortingLayer < spriteB.sortingLayer;
                  }
                  return spriteA.sortingOrder < spriteB.sortingOrder;
              });

    m_sortDirty = false;
}

void SpriteRenderSystem::update(World& world, f32 /*deltaTime*/) {
    if (!m_camera) {
        return;
    }

    // Rebuild sorted list only when dirty
    if (m_sortDirty) {
        rebuildSortedList(world);
    }

    // Begin rendering
    Renderer2D::beginScene(*m_camera);

    // Render all sprites using cached sorted order
    auto view = world.view<TransformComponent, SpriteRendererComponent>();

    for (auto entity : m_sortedEntities) {
        // Validate entity still exists and has required components
        if (!world.isValid(entity) || !view.contains(entity)) {
            // Entity was removed but we haven't rebuilt yet - mark dirty for next frame
            m_sortDirty = true;
            continue;
        }

        const auto& transform = view.get<TransformComponent>(entity);
        const auto& sprite = view.get<SpriteRendererComponent>(entity);

        // Build transform matrix
        glm::mat4 const transformMatrix = transform.getMatrix();

        // Draw the quad with the sprite's color
        // TODO: Support textured sprites via AssetId lookup
        Renderer2D::drawQuad(transformMatrix, sprite.color);
    }

    Renderer2D::endScene();
}

}  // namespace limbo
