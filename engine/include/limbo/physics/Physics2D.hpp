#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>
#include <box2d/box2d.h>

namespace limbo {

/**
 * Physics2D - 2D Physics world wrapper around Box2D
 *
 * Provides a simplified interface to Box2D for 2D physics simulation.
 * Handles world creation, stepping, and body management.
 *
 * Usage:
 *   Physics2D physics;
 *   physics.init();
 *   physics.setGravity({0.0f, -9.81f});
 *
 *   // In update loop:
 *   physics.step(deltaTime);
 */
class LIMBO_API Physics2D {
public:
    Physics2D() = default;
    ~Physics2D();

    // Non-copyable
    Physics2D(const Physics2D&) = delete;
    Physics2D& operator=(const Physics2D&) = delete;

    /**
     * Initialize the physics world
     * @param gravity Initial gravity vector (default: 0, -9.81)
     */
    void init(const glm::vec2& gravity = {0.0f, -9.81f});

    /**
     * Shutdown and cleanup the physics world
     */
    void shutdown();

    /**
     * Step the physics simulation
     * @param deltaTime Time step in seconds
     */
    void step(f32 deltaTime);

    /**
     * Set the gravity vector
     */
    void setGravity(const glm::vec2& gravity);

    /**
     * Get the current gravity vector
     */
    [[nodiscard]] glm::vec2 getGravity() const;

    /**
     * Get the underlying Box2D world (for advanced usage)
     */
    [[nodiscard]] b2World* getWorld() { return m_world; }
    [[nodiscard]] const b2World* getWorld() const { return m_world; }

    /**
     * Check if physics is initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_world != nullptr; }

    /**
     * Set velocity iterations for constraint solver
     */
    void setVelocityIterations(i32 iterations) { m_velocityIterations = iterations; }

    /**
     * Set position iterations for constraint solver
     */
    void setPositionIterations(i32 iterations) { m_positionIterations = iterations; }

    /**
     * Convert from world units to physics units (pixels to meters)
     */
    static f32 toPhysics(f32 worldUnits) { return worldUnits * s_pixelsToMeters; }
    static glm::vec2 toPhysics(const glm::vec2& worldUnits) {
        return worldUnits * s_pixelsToMeters;
    }

    /**
     * Convert from physics units to world units (meters to pixels)
     */
    static f32 toWorld(f32 physicsUnits) { return physicsUnits * s_metersToPixels; }
    static glm::vec2 toWorld(const glm::vec2& physicsUnits) {
        return physicsUnits * s_metersToPixels;
    }

    /**
     * Set the pixels-to-meters conversion factor
     * Default is 1.0 (1 world unit = 1 meter)
     */
    static void setPixelsPerMeter(f32 ppm) {
        s_pixelsToMeters = 1.0f / ppm;
        s_metersToPixels = ppm;
    }

private:
    b2World* m_world = nullptr;
    i32 m_velocityIterations = 8;
    i32 m_positionIterations = 3;

    // Conversion factors (default: 1 unit = 1 meter)
    static inline f32 s_pixelsToMeters = 1.0f;
    static inline f32 s_metersToPixels = 1.0f;
};

}  // namespace limbo
