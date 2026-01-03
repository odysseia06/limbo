#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>
#include <box2d/box2d.h>

namespace limbo {

/**
 * Rigidbody2D types
 */
enum class BodyType : u8 {
    Static = 0,     // Does not move, infinite mass
    Kinematic,      // Moves via velocity, not affected by forces
    Dynamic         // Fully simulated, affected by forces and collisions
};

/**
 * Rigidbody2DComponent - Represents a physics body
 *
 * Attach to an entity to give it physics simulation.
 * Requires a Collider2DComponent to define its shape.
 */
struct LIMBO_API Rigidbody2DComponent {
    BodyType type = BodyType::Dynamic;
    
    // Physical properties
    f32 gravityScale = 1.0f;
    bool fixedRotation = false;
    
    // Initial velocity (applied when body is created)
    glm::vec2 linearVelocity{0.0f};
    f32 angularVelocity = 0.0f;
    
    // Linear and angular damping (0 = no damping)
    f32 linearDamping = 0.0f;
    f32 angularDamping = 0.01f;
    
    // Runtime data (managed by PhysicsSystem)
    b2Body* runtimeBody = nullptr;

    Rigidbody2DComponent() = default;
    explicit Rigidbody2DComponent(BodyType bodyType) : type(bodyType) {}
};

/**
 * Collider2D shape types
 */
enum class ColliderShape : u8 {
    Box = 0,
    Circle
};

/**
 * BoxCollider2DComponent - Box-shaped collider
 */
struct LIMBO_API BoxCollider2DComponent {
    // Half-extents of the box (size/2)
    glm::vec2 size{0.5f, 0.5f};
    
    // Offset from entity center
    glm::vec2 offset{0.0f, 0.0f};
    
    // Physical material properties
    f32 density = 1.0f;
    f32 friction = 0.3f;
    f32 restitution = 0.0f;      // Bounciness (0 = no bounce, 1 = perfect bounce)
    f32 restitutionThreshold = 0.5f;
    
    // Is this a trigger (no physical collision, just detection)?
    bool isTrigger = false;
    
    // Runtime data
    b2Fixture* runtimeFixture = nullptr;

    BoxCollider2DComponent() = default;
    explicit BoxCollider2DComponent(const glm::vec2& halfSize) : size(halfSize) {}
};

/**
 * CircleCollider2DComponent - Circle-shaped collider
 */
struct LIMBO_API CircleCollider2DComponent {
    // Radius of the circle
    f32 radius = 0.5f;
    
    // Offset from entity center
    glm::vec2 offset{0.0f, 0.0f};
    
    // Physical material properties
    f32 density = 1.0f;
    f32 friction = 0.3f;
    f32 restitution = 0.0f;
    f32 restitutionThreshold = 0.5f;
    
    // Is this a trigger?
    bool isTrigger = false;
    
    // Runtime data
    b2Fixture* runtimeFixture = nullptr;

    CircleCollider2DComponent() = default;
    explicit CircleCollider2DComponent(f32 r) : radius(r) {}
};

} // namespace limbo
