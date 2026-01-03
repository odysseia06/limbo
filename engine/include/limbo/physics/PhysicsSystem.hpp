#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/physics/Physics2D.hpp"

namespace limbo {

/**
 * PhysicsSystem - Manages physics simulation for entities
 *
 * Handles:
 * - Creating Box2D bodies for entities with Rigidbody2DComponent
 * - Creating fixtures for collider components
 * - Stepping the physics simulation
 * - Syncing transform positions with physics bodies
 *
 * Usage:
 *   getSystems().addSystem<PhysicsSystem>(physics2D);
 */
class LIMBO_API PhysicsSystem : public System {
public:
    explicit PhysicsSystem(Physics2D& physics);
    ~PhysicsSystem() override = default;

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

} // namespace limbo
