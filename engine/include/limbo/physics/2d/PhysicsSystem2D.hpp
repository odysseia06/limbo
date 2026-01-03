#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/physics/2d/Physics2D.hpp"

namespace limbo {

/**
 * PhysicsSystem2D - Manages 2D physics simulation for entities
 *
 * Handles:
 * - Creating Box2D bodies for entities with Rigidbody2DComponent
 * - Creating fixtures for collider components
 * - Stepping the physics simulation
 * - Syncing transform positions with physics bodies
 *
 * Usage:
 *   getSystems().addSystem<PhysicsSystem2D>(physics2D);
 */
class LIMBO_API PhysicsSystem2D : public System {
public:
    explicit PhysicsSystem2D(Physics2D& physics);
    ~PhysicsSystem2D() override = default;

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;

private:
    void createBody(World& world, World::EntityId entity);
    void destroyBody(World& world, World::EntityId entity);
    void syncTransformToBody(World& world, World::EntityId entity);
    void syncBodyToTransform(World& world, World::EntityId entity);

    Physics2D& m_physics;
};

}  // namespace limbo
