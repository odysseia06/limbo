#include "limbo/ecs/systems/2d/SpriteRenderSystem.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/2d/SpriteMaterial.hpp"
#include "limbo/render/common/Camera.hpp"
#include "limbo/assets/AssetManager.hpp"
#include "limbo/assets/TextureAsset.hpp"

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

    // Watch SpriteMaterialComponent for material-based sprites
    registry.on_construct<SpriteMaterialComponent>().connect<&SpriteRenderSystem::markDirty>(this);
    registry.on_destroy<SpriteMaterialComponent>().connect<&SpriteRenderSystem::markDirty>(this);

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
    registry.on_construct<SpriteMaterialComponent>().disconnect<&SpriteRenderSystem::markDirty>(
        this);
    registry.on_destroy<SpriteMaterialComponent>().disconnect<&SpriteRenderSystem::markDirty>(this);

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

void SpriteRenderSystem::renderBatchedSprite(World& world, World::EntityId entity) {
    const auto& transform = world.getComponent<TransformComponent>(entity);
    const auto& sprite = world.getComponent<SpriteRendererComponent>(entity);

    glm::mat4 const transformMatrix = transform.getMatrix();

    // Check if we have a texture
    if (sprite.textureId.isValid() && m_assetManager != nullptr) {
        auto textureAsset = m_assetManager->get<TextureAsset>(sprite.textureId);
        if (textureAsset != nullptr && textureAsset->getTexture() != nullptr) {
            // Check for custom UVs (sprite sheet support)
            if (sprite.uvMin != glm::vec2(0.0f) || sprite.uvMax != glm::vec2(1.0f)) {
                Renderer2D::drawQuad(transformMatrix, *textureAsset->getTexture(), sprite.uvMin,
                                     sprite.uvMax, sprite.color);
            } else {
                Renderer2D::drawQuad(transformMatrix, *textureAsset->getTexture(), 1.0f,
                                     sprite.color);
            }
            return;
        }
    }

    // No texture or failed to load - draw colored quad
    Renderer2D::drawQuad(transformMatrix, sprite.color);
}

void SpriteRenderSystem::renderMaterialSprite(World& world, World::EntityId entity) {
    const auto& transform = world.getComponent<TransformComponent>(entity);
    const auto& sprite = world.getComponent<SpriteRendererComponent>(entity);
    const auto& materialComp = world.getComponent<SpriteMaterialComponent>(entity);

    if (!materialComp.material) {
        // No material - fall back to batched rendering
        renderBatchedSprite(world, entity);
        return;
    }

    // Flush current batch before switching to custom material
    Renderer2D::flush();

    // Set up material properties from sprite component
    SpriteMaterial& material = *materialComp.material;
    material.setColor(sprite.color);

    // Resolve and set texture if present
    Texture2D* texture = nullptr;
    if (sprite.textureId.isValid() && m_assetManager != nullptr) {
        auto textureAsset = m_assetManager->get<TextureAsset>(sprite.textureId);
        if (textureAsset != nullptr && textureAsset->getTexture() != nullptr) {
            texture = textureAsset->getTexture();
            material.setTexture(texture);
        }
    }

    // Bind material (activates custom shader and sets uniforms)
    material.bind();

    glm::mat4 const transformMatrix = transform.getMatrix();

    // Use immediate draw path so the material's shader is actually used
    // (batched drawQuad would use the built-in batch shader on flush)
    Renderer2D::drawQuadImmediate(transformMatrix, texture, sprite.color);

    material.unbind();
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
    for (auto entity : m_sortedEntities) {
        // Validate entity still exists and has required components
        if (!world.isValid(entity) || !world.hasComponent<SpriteRendererComponent>(entity) ||
            !world.hasComponent<TransformComponent>(entity)) {
            // Entity was removed or lost required component - mark dirty for next frame
            m_sortDirty = true;
            continue;
        }

        // Check if entity has a custom material
        if (world.hasComponent<SpriteMaterialComponent>(entity)) {
            renderMaterialSprite(world, entity);
        } else {
            renderBatchedSprite(world, entity);
        }
    }

    Renderer2D::endScene();
}

}  // namespace limbo
