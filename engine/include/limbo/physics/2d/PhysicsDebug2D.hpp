#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>

// Forward declaration
class b2World;

namespace limbo {

// Forward declaration
class Physics2D;

/**
 * PhysicsDebug2D - Debug visualization for Box2D physics
 *
 * Draws physics shapes directly from the Box2D world (not ECS components)
 * to show the actual physics state as the simulation sees it.
 *
 * Usage:
 *   PhysicsDebug2D debugDraw;
 *   debugDraw.draw(physics2D);  // Call during render pass
 */
class LIMBO_API PhysicsDebug2D {
public:
    PhysicsDebug2D() = default;
    ~PhysicsDebug2D() = default;

    /**
     * Draw all physics bodies and shapes
     * @param physics The Physics2D system to visualize
     */
    void draw(const Physics2D& physics);

    /**
     * Draw physics from a raw Box2D world
     * @param world The Box2D world to visualize
     */
    void draw(const b2World* world);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Enable/disable debug drawing
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * Show/hide static bodies (default: true)
     */
    void setDrawStaticBodies(bool draw) { m_drawStaticBodies = draw; }
    [[nodiscard]] bool getDrawStaticBodies() const { return m_drawStaticBodies; }

    /**
     * Show/hide kinematic bodies (default: true)
     */
    void setDrawKinematicBodies(bool draw) { m_drawKinematicBodies = draw; }
    [[nodiscard]] bool getDrawKinematicBodies() const { return m_drawKinematicBodies; }

    /**
     * Show/hide dynamic bodies (default: true)
     */
    void setDrawDynamicBodies(bool draw) { m_drawDynamicBodies = draw; }
    [[nodiscard]] bool getDrawDynamicBodies() const { return m_drawDynamicBodies; }

    /**
     * Show/hide sensors/triggers (default: true)
     */
    void setDrawSensors(bool draw) { m_drawSensors = draw; }
    [[nodiscard]] bool getDrawSensors() const { return m_drawSensors; }

    /**
     * Show/hide AABBs (default: false)
     */
    void setDrawAABBs(bool draw) { m_drawAABBs = draw; }
    [[nodiscard]] bool getDrawAABBs() const { return m_drawAABBs; }

    /**
     * Show/hide body center of mass (default: false)
     */
    void setDrawCenterOfMass(bool draw) { m_drawCenterOfMass = draw; }
    [[nodiscard]] bool getDrawCenterOfMass() const { return m_drawCenterOfMass; }

    // ========================================================================
    // Colors
    // ========================================================================

    void setStaticBodyColor(const glm::vec4& color) { m_staticBodyColor = color; }
    void setKinematicBodyColor(const glm::vec4& color) { m_kinematicBodyColor = color; }
    void setDynamicBodyColor(const glm::vec4& color) { m_dynamicBodyColor = color; }
    void setSensorColor(const glm::vec4& color) { m_sensorColor = color; }
    void setAABBColor(const glm::vec4& color) { m_aabbColor = color; }
    void setCenterOfMassColor(const glm::vec4& color) { m_centerOfMassColor = color; }

private:
    bool m_enabled = true;
    bool m_drawStaticBodies = true;
    bool m_drawKinematicBodies = true;
    bool m_drawDynamicBodies = true;
    bool m_drawSensors = true;
    bool m_drawAABBs = false;
    bool m_drawCenterOfMass = false;

    // Default colors (RGBA)
    glm::vec4 m_staticBodyColor{0.5f, 0.5f, 0.5f, 1.0f};      // Gray
    glm::vec4 m_kinematicBodyColor{0.5f, 0.5f, 0.9f, 1.0f};   // Light blue
    glm::vec4 m_dynamicBodyColor{0.0f, 1.0f, 0.0f, 1.0f};     // Green
    glm::vec4 m_sensorColor{1.0f, 1.0f, 0.0f, 0.5f};          // Yellow (transparent)
    glm::vec4 m_aabbColor{1.0f, 0.0f, 1.0f, 0.5f};            // Magenta (transparent)
    glm::vec4 m_centerOfMassColor{1.0f, 0.0f, 0.0f, 1.0f};    // Red
};

}  // namespace limbo
