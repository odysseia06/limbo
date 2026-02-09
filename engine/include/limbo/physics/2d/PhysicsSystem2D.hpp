#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/physics/2d/Physics2D.hpp"
#include "limbo/physics/2d/ContactListener2D.hpp"

#include <entt/entt.hpp>

#include <unordered_map>

namespace limbo {

/**
 * PhysicsSystem2D - Manages 2D physics simulation for entities
 *
 * Handles:
 * - Creating Box2D bodies for entities with Rigidbody2DComponent
 * - Creating fixtures for collider components
 * - Fixed timestep physics simulation with interpolation
 * - Syncing transform positions with physics bodies
 *
 * Fixed Timestep:
 * Physics runs at a fixed rate (default 60Hz) for determinism.
 * Render transforms are interpolated between physics states.
 * The interpolated pose NEVER affects simulation - it's render-only.
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

    /**
     * Set the collision callback for physics events
     * Called after physics step with self-relative collision data
     */
    void setCollisionCallback(CollisionCallback callback);

    /**
     * Get the contact listener (for advanced usage)
     */
    [[nodiscard]] ContactListener2D& getContactListener() { return m_contactListener; }

    /**
     * Set the fixed timestep for physics simulation
     * @param timestep Fixed delta time (default: 1/60 = 0.01667s)
     */
    void setFixedTimestep(f32 timestep) { m_fixedTimestep = timestep; }

    /**
     * Get the current fixed timestep
     */
    [[nodiscard]] f32 getFixedTimestep() const { return m_fixedTimestep; }

    /**
     * Set maximum fixed updates per frame (spiral-of-death protection)
     * @param maxUpdates Maximum physics steps per frame (default: 8)
     */
    void setMaxFixedUpdatesPerFrame(i32 maxUpdates) { m_maxFixedUpdatesPerFrame = maxUpdates; }

    /**
     * Enable/disable interpolation for smoother rendering
     * When disabled, transforms update directly from physics (no smoothing)
     */
    void setInterpolationEnabled(bool enabled) { m_interpolationEnabled = enabled; }

    /**
     * Check if interpolation is enabled
     */
    [[nodiscard]] bool isInterpolationEnabled() const { return m_interpolationEnabled; }

private:
    void createBody(World& world, World::EntityId entity);
    void destroyBody(World& world, World::EntityId entity);
    void syncTransformToBody(World& world, World::EntityId entity);

    void runFixedUpdate(World& world);
    void snapshotPreviousState();
    void readCurrentStateFromBodies(World& world);
    void interpolateRenderState(World& world, f32 alpha);

    // Signal handler for component destruction
    void onRigidbodyDestroyed(entt::registry& registry, entt::entity entity);

    Physics2D& m_physics;
    ContactListener2D m_contactListener;
    World* m_world = nullptr;

    // Fixed timestep state
    f32 m_fixedTimestep = 1.0f / 60.0f;
    f32 m_accumulator = 0.0f;
    i32 m_maxFixedUpdatesPerFrame = 8;
    bool m_interpolationEnabled = true;

    // Internal physics state cache (separate from render state in TransformComponent)
    struct PhysicsState {
        glm::vec2 previousPosition{0.0f};
        f32 previousRotation = 0.0f;
        glm::vec2 currentPosition{0.0f};
        f32 currentRotation = 0.0f;
    };
    std::unordered_map<World::EntityId, PhysicsState> m_physicsStates;
};

}  // namespace limbo
