#pragma once

#include <limbo/Limbo.hpp>

#include <glm/glm.hpp>

namespace limbo::editor {

/**
 * GizmoMode - The type of gizmo operation
 */
enum class GizmoMode { Translate, Rotate, Scale };

/**
 * GizmoSpace - The coordinate space for gizmo operations
 */
enum class GizmoSpace { Local, World };

/**
 * GizmoAxis - Which axis is being manipulated
 */
enum class GizmoAxis { None, X, Y, Z, XY, XZ, YZ, XYZ };

/**
 * Gizmo - Handles visual gizmo rendering and interaction for transforms
 *
 * Provides translation, rotation, and scale gizmos for manipulating
 * entity transforms in the viewport.
 */
class Gizmo {
public:
    Gizmo() = default;

    /**
     * Set the current gizmo mode
     */
    void setMode(GizmoMode mode) { m_mode = mode; }
    [[nodiscard]] GizmoMode getMode() const { return m_mode; }

    /**
     * Set the coordinate space
     */
    void setSpace(GizmoSpace space) { m_space = space; }
    [[nodiscard]] GizmoSpace getSpace() const { return m_space; }

    /**
     * Enable or disable snapping
     */
    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    [[nodiscard]] bool isSnapEnabled() const { return m_snapEnabled; }

    /**
     * Set snap values for each mode
     */
    void setTranslateSnap(f32 snap) { m_translateSnap = snap; }
    void setRotateSnap(f32 snap) { m_rotateSnap = snap; }
    void setScaleSnap(f32 snap) { m_scaleSnap = snap; }

    [[nodiscard]] f32 getTranslateSnap() const { return m_translateSnap; }
    [[nodiscard]] f32 getRotateSnap() const { return m_rotateSnap; }
    [[nodiscard]] f32 getScaleSnap() const { return m_scaleSnap; }

    /**
     * Begin a gizmo manipulation
     * @param position The starting position in world space
     * @param rotation The starting rotation in world space
     * @param scale The starting scale
     * @param mousePos The starting mouse position in screen space
     */
    void begin(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale,
               const glm::vec2& mousePos);

    /**
     * Update the gizmo with current mouse position
     * @param mousePos The current mouse position in screen space
     * @param viewportSize The viewport size
     * @param camera The camera for projection
     * @return True if the gizmo is being manipulated
     */
    bool update(const glm::vec2& mousePos, const glm::vec2& viewportSize,
                const OrthographicCamera& camera);

    /**
     * End gizmo manipulation
     */
    void end();

    /**
     * Check if gizmo is currently being manipulated
     */
    [[nodiscard]] bool isManipulating() const { return m_isManipulating; }

    /**
     * Get the current axis being manipulated
     */
    [[nodiscard]] GizmoAxis getActiveAxis() const { return m_activeAxis; }

    /**
     * Get the delta transform from the manipulation
     */
    [[nodiscard]] glm::vec3 getPositionDelta() const { return m_positionDelta; }
    [[nodiscard]] glm::vec3 getRotationDelta() const { return m_rotationDelta; }
    [[nodiscard]] glm::vec3 getScaleDelta() const { return m_scaleDelta; }

    /**
     * Get the current (accumulated) transform values
     */
    [[nodiscard]] glm::vec3 getCurrentPosition() const { return m_currentPosition; }
    [[nodiscard]] glm::vec3 getCurrentRotation() const { return m_currentRotation; }
    [[nodiscard]] glm::vec3 getCurrentScale() const { return m_currentScale; }

    /**
     * Draw the gizmo
     * @param position The center position of the gizmo
     * @param rotation The rotation (for local space)
     * @param scale The scale
     * @param cameraZoom The camera zoom for consistent visual size
     */
    void draw(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale,
              f32 cameraZoom);

    /**
     * Check if a point is over the gizmo
     * @param mousePos Mouse position in world space
     * @param position Gizmo position
     * @param cameraZoom Camera zoom for hit testing
     * @return The axis that would be selected, or None
     */
    GizmoAxis hitTest(const glm::vec2& mousePos, const glm::vec3& position, f32 cameraZoom);

private:
    void drawTranslateGizmo(const glm::vec3& position, f32 size);
    void drawRotateGizmo(const glm::vec3& position, f32 size);
    void drawScaleGizmo(const glm::vec3& position, f32 size);

    f32 snapValue(f32 value, f32 snap) const;

private:
    GizmoMode m_mode = GizmoMode::Translate;
    GizmoSpace m_space = GizmoSpace::World;
    GizmoAxis m_activeAxis = GizmoAxis::None;
    GizmoAxis m_hoveredAxis = GizmoAxis::None;

    bool m_isManipulating = false;
    bool m_snapEnabled = false;

    // Snap values
    f32 m_translateSnap = 0.5f;
    f32 m_rotateSnap = 15.0f;  // degrees
    f32 m_scaleSnap = 0.1f;

    // Starting values
    glm::vec3 m_startPosition{0.0f};
    glm::vec3 m_startRotation{0.0f};
    glm::vec3 m_startScale{1.0f};
    glm::vec2 m_startMousePos{0.0f};

    // Current values
    glm::vec3 m_currentPosition{0.0f};
    glm::vec3 m_currentRotation{0.0f};
    glm::vec3 m_currentScale{1.0f};

    // Delta values (this frame)
    glm::vec3 m_positionDelta{0.0f};
    glm::vec3 m_rotationDelta{0.0f};
    glm::vec3 m_scaleDelta{0.0f};

    // Visual settings
    f32 m_gizmoSize = 0.15f;  // Size relative to zoom
    f32 m_axisLength = 1.0f;
    f32 m_axisThickness = 0.02f;
};

}  // namespace limbo::editor
