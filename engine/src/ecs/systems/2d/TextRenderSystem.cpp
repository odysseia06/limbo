#include "limbo/ecs/systems/2d/TextRenderSystem.hpp"

#include "limbo/assets/AssetManager.hpp"
#include "limbo/assets/FontAsset.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/2d/TextRenderer.hpp"
#include "limbo/render/common/Camera.hpp"

#include <algorithm>

namespace limbo {

void TextRenderSystem::onAttach(World& world) {
    // Subscribe to component add/remove via EnTT signals to mark dirty
    auto& registry = world.registry();

    // When TextRendererComponent is added or removed, we need to re-sort
    registry.on_construct<TextRendererComponent>().connect<&TextRenderSystem::markDirty>(this);
    registry.on_destroy<TextRendererComponent>().connect<&TextRenderSystem::markDirty>(this);

    // Also watch TransformComponent since entities need both
    registry.on_construct<TransformComponent>().connect<&TextRenderSystem::markDirty>(this);
    registry.on_destroy<TransformComponent>().connect<&TextRenderSystem::markDirty>(this);

    m_sortDirty = true;
}

void TextRenderSystem::onDetach(World& world) {
    // Disconnect signals
    auto& registry = world.registry();

    registry.on_construct<TextRendererComponent>().disconnect<&TextRenderSystem::markDirty>(this);
    registry.on_destroy<TextRendererComponent>().disconnect<&TextRenderSystem::markDirty>(this);
    registry.on_construct<TransformComponent>().disconnect<&TextRenderSystem::markDirty>(this);
    registry.on_destroy<TransformComponent>().disconnect<&TextRenderSystem::markDirty>(this);

    m_sortedEntities.clear();
}

void TextRenderSystem::rebuildSortedList(World& world) {
    m_sortedEntities.clear();

    auto view = world.view<TransformComponent, TextRendererComponent>();

    // Reserve capacity
    m_sortedEntities.reserve(64);

    for (auto entity : view) {
        m_sortedEntities.push_back(entity);
    }

    // Sort by layer first, then by order within layer
    std::sort(m_sortedEntities.begin(), m_sortedEntities.end(),
              [&view](World::EntityId a, World::EntityId b) {
                  const auto& textA = view.get<TextRendererComponent>(a);
                  const auto& textB = view.get<TextRendererComponent>(b);

                  if (textA.sortingLayer != textB.sortingLayer) {
                      return textA.sortingLayer < textB.sortingLayer;
                  }
                  return textA.sortingOrder < textB.sortingOrder;
              });

    m_sortDirty = false;
}

void TextRenderSystem::update(World& world, f32 /*deltaTime*/) {
    if (!m_camera || !m_assetManager) {
        return;
    }

    // Rebuild sorted list only when dirty
    if (m_sortDirty) {
        rebuildSortedList(world);
    }

    // Render all text entities using cached sorted order
    auto view = world.view<TransformComponent, TextRendererComponent>();

    for (auto entity : m_sortedEntities) {
        // Validate entity still exists and has required components
        if (!world.isValid(entity) || !view.contains(entity)) {
            // Entity was removed but we haven't rebuilt yet - mark dirty for next frame
            m_sortDirty = true;
            continue;
        }

        const auto& transform = view.get<TransformComponent>(entity);
        const auto& textComp = view.get<TextRendererComponent>(entity);

        // Skip if no text or no font
        if (textComp.text.empty() || !textComp.fontId.isValid()) {
            continue;
        }

        // Get the font from asset manager
        auto fontAsset = m_assetManager->get<FontAsset>(textComp.fontId);
        if (!fontAsset || !fontAsset->isLoaded() || !fontAsset->getFont()) {
            continue;
        }

        const Font* font = fontAsset->getFont();

        // Draw text at entity position
        // Use vec3 position with Z for depth sorting
        glm::vec3 const position = transform.position;

        TextRenderer::drawText(textComp.text, position, *font, textComp.scale, textComp.color);
    }
}

}  // namespace limbo
