#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"

namespace limbo {

/**
 * AnimationSystem - ECS system that updates sprite animations
 *
 * Updates AnimatorComponent states and syncs sprite UVs to the current frame.
 */
class LIMBO_API AnimationSystem : public System {
public:
    AnimationSystem() = default;
    ~AnimationSystem() override = default;

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;
};

} // namespace limbo
