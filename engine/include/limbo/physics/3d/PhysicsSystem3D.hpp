#pragma once

#include "limbo/ecs/System.hpp"
#include "limbo/physics/3d/Physics3D.hpp"

namespace limbo {

/**
 * @brief System for 3D physics simulation
 *
 * PhysicsSystem3D manages the lifecycle of physics bodies and syncs
 * transforms between the ECS and physics engine.
 *
 * NOTE: This is a skeleton for future implementation.
 */
class PhysicsSystem3D : public System {
public:
    explicit PhysicsSystem3D(Physics3D& physics);
    ~PhysicsSystem3D() override = default;

    void onAttach(World& world) override;
    void onDetach() override;
    void update(float deltaTime) override;
    void fixedUpdate(float fixedDeltaTime) override;

private:
    World* m_world = nullptr;
    Physics3D& m_physics;

    void createBody(Entity entity);
    void destroyBody(Entity entity);
    void syncTransformsToPhysics();
    void syncTransformsFromPhysics();
};

}  // namespace limbo
