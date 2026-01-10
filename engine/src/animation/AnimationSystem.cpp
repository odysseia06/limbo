#include "limbo/animation/AnimationSystem.hpp"

#include "limbo/animation/AnimatorComponent.hpp"
#include "limbo/debug/Log.hpp"
#include "limbo/ecs/Components.hpp"

namespace limbo {

void AnimationSystem::onAttach(World& /*world*/) {
    LIMBO_LOG_CORE_DEBUG("AnimationSystem initialized");
}

void AnimationSystem::update(World& world, f32 deltaTime) {
    world.each<AnimatorComponent>(
        [&world, deltaTime](World::EntityId entity, AnimatorComponent& animator) {
            // Handle first-time initialization
            if (animator.playOnStart && !animator.currentState.getClip()) {
                if (!animator.defaultClip.empty()) {
                    animator.play(animator.defaultClip);
                    animator.playOnStart = false;
                }
            }

            // Update animation state
            animator.currentState.update(deltaTime);

            // Sync sprite UVs if entity has SpriteRendererComponent
            if (world.hasComponent<SpriteRendererComponent>(entity)) {
                auto& sprite = world.getComponent<SpriteRendererComponent>(entity);
                const SpriteFrame* frame = animator.getCurrentFrame();

                if (frame) {
                    sprite.uvMin = frame->uvMin;
                    sprite.uvMax = frame->uvMax;
                }
            }
        });
}

void AnimationSystem::onDetach(World& /*world*/) {
    LIMBO_LOG_CORE_DEBUG("AnimationSystem shutdown");
}

}  // namespace limbo
