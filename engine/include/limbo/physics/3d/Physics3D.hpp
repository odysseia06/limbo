#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/ecs/Entity.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

/**
 * @brief 3D Physics world wrapper
 *
 * Physics3D wraps a 3D physics engine (e.g., Jolt Physics or Bullet)
 * providing collision detection and rigid body dynamics.
 *
 * NOTE: This is a skeleton for future implementation.
 */
class Physics3D {
public:
    Physics3D(const glm::vec3& gravity = glm::vec3(0.0f, -9.81f, 0.0f));
    ~Physics3D();

    // Non-copyable
    Physics3D(const Physics3D&) = delete;
    Physics3D& operator=(const Physics3D&) = delete;

    /// Step the physics simulation
    void step(float deltaTime);

    /// Set gravity vector
    void setGravity(const glm::vec3& gravity);

    /// Get gravity vector
    glm::vec3 getGravity() const;

    // Raycasting
    struct RaycastHit {
        Entity entity;
        glm::vec3 point{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        float distance = 0.0f;
        bool hit = false;
    };

    /// Cast a ray and return the first hit
    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction,
                       float maxDistance = 1000.0f);

    /// Cast a ray and return all hits
    std::vector<RaycastHit> raycastAll(const glm::vec3& origin, const glm::vec3& direction,
                                       float maxDistance = 1000.0f);

    // Shape queries
    std::vector<Entity> overlapSphere(const glm::vec3& center, float radius);
    std::vector<Entity> overlapBox(const glm::vec3& center, const glm::vec3& halfExtents);

private:
    glm::vec3 m_gravity;

    // Physics engine internals (Jolt/Bullet)
    // void* m_physicsSystem = nullptr;
};

}  // namespace limbo
