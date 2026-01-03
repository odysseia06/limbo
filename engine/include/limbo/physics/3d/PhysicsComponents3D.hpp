#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"

#include <glm/glm.hpp>

namespace limbo {

/**
 * @brief 3D Rigidbody component
 */
struct Rigidbody3DComponent {
    enum class Type { Static, Kinematic, Dynamic };

    Type type = Type::Dynamic;
    float mass = 1.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.05f;
    bool useGravity = true;
    bool isKinematic = false;

    // Constraints
    bool freezePositionX = false;
    bool freezePositionY = false;
    bool freezePositionZ = false;
    bool freezeRotationX = false;
    bool freezeRotationY = false;
    bool freezeRotationZ = false;

    // Initial velocities
    glm::vec3 linearVelocity{0.0f};
    glm::vec3 angularVelocity{0.0f};

    // Runtime physics body (set by PhysicsSystem3D)
    void* runtimeBody = nullptr;
};

/**
 * @brief 3D Box collider component
 */
struct BoxCollider3DComponent {
    glm::vec3 halfExtents{0.5f};
    glm::vec3 offset{0.0f};

    // Physics material
    float friction = 0.5f;
    float restitution = 0.0f;
    float density = 1.0f;

    bool isTrigger = false;

    // Runtime fixture (set by PhysicsSystem3D)
    void* runtimeFixture = nullptr;
};

/**
 * @brief 3D Sphere collider component
 */
struct SphereCollider3DComponent {
    float radius = 0.5f;
    glm::vec3 offset{0.0f};

    // Physics material
    float friction = 0.5f;
    float restitution = 0.0f;
    float density = 1.0f;

    bool isTrigger = false;

    void* runtimeFixture = nullptr;
};

/**
 * @brief 3D Capsule collider component
 */
struct CapsuleCollider3DComponent {
    float radius = 0.5f;
    float height = 2.0f;  // Total height including hemispheres
    glm::vec3 offset{0.0f};

    enum class Direction { X, Y, Z };
    Direction direction = Direction::Y;

    // Physics material
    float friction = 0.5f;
    float restitution = 0.0f;
    float density = 1.0f;

    bool isTrigger = false;

    void* runtimeFixture = nullptr;
};

/**
 * @brief 3D Mesh collider component
 */
struct MeshCollider3DComponent {
    AssetId meshId;
    bool convex = false;  // Convex for dynamic bodies, concave for static only

    // Physics material
    float friction = 0.5f;
    float restitution = 0.0f;

    bool isTrigger = false;

    void* runtimeFixture = nullptr;
};

/**
 * @brief 3D Cylinder collider component
 */
struct CylinderCollider3DComponent {
    float radius = 0.5f;
    float height = 1.0f;
    glm::vec3 offset{0.0f};

    enum class Direction { X, Y, Z };
    Direction direction = Direction::Y;

    // Physics material
    float friction = 0.5f;
    float restitution = 0.0f;
    float density = 1.0f;

    bool isTrigger = false;

    void* runtimeFixture = nullptr;
};

}  // namespace limbo
