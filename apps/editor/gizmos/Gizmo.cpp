#include "Gizmo.hpp"

#include <limbo/render/2d/Renderer2D.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace limbo::editor {

namespace {

// Gizmo colors
const glm::vec4 kAxisColorX(0.9f, 0.2f, 0.2f, 1.0f);
const glm::vec4 kAxisColorY(0.2f, 0.9f, 0.2f, 1.0f);
const glm::vec4 kAxisColorZ(0.2f, 0.2f, 0.9f, 1.0f);
const glm::vec4 kAxisColorXY(0.9f, 0.9f, 0.2f, 0.5f);
const glm::vec4 kAxisColorHighlight(1.0f, 1.0f, 0.0f, 1.0f);
const glm::vec4 kCenterColor(0.8f, 0.8f, 0.8f, 1.0f);

}  // namespace

void Gizmo::begin(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale,
                  const glm::vec2& mousePos) {
    m_startPosition = position;
    m_startRotation = rotation;
    m_startScale = scale;
    m_currentPosition = position;
    m_currentRotation = rotation;
    m_currentScale = scale;
    m_positionDelta = glm::vec3(0.0f);
    m_rotationDelta = glm::vec3(0.0f);
    m_scaleDelta = glm::vec3(0.0f);
    m_startMousePos = mousePos;
    m_isManipulating = true;
    m_activeAxis = m_hoveredAxis;  // Lock to the hovered axis
}

bool Gizmo::update(const glm::vec2& mousePos, const glm::vec2& viewportSize,
                   const OrthographicCamera& camera) {
    if (!m_isManipulating || m_activeAxis == GizmoAxis::None) {
        return false;
    }

    // Convert mouse position to world space
    glm::vec2 normalizedPos = mousePos / viewportSize * 2.0f - 1.0f;
    normalizedPos.y = -normalizedPos.y;

    glm::mat4 invViewProj = glm::inverse(camera.getViewProjectionMatrix());
    glm::vec4 worldPos4 = invViewProj * glm::vec4(normalizedPos, 0.0f, 1.0f);
    glm::vec2 worldPos(worldPos4.x, worldPos4.y);

    // Calculate delta from start
    glm::vec2 startNormalized = m_startMousePos / viewportSize * 2.0f - 1.0f;
    startNormalized.y = -startNormalized.y;
    glm::vec4 startWorld4 = invViewProj * glm::vec4(startNormalized, 0.0f, 1.0f);
    glm::vec2 startWorld(startWorld4.x, startWorld4.y);

    glm::vec2 delta = worldPos - startWorld;

    switch (m_mode) {
        case GizmoMode::Translate: {
            glm::vec3 translation(0.0f);

            switch (m_activeAxis) {
                case GizmoAxis::X:
                    translation.x = delta.x;
                    break;
                case GizmoAxis::Y:
                    translation.y = delta.y;
                    break;
                case GizmoAxis::XY:
                    translation.x = delta.x;
                    translation.y = delta.y;
                    break;
                default:
                    break;
            }

            if (m_snapEnabled) {
                translation.x = snapValue(translation.x, m_translateSnap);
                translation.y = snapValue(translation.y, m_translateSnap);
                translation.z = snapValue(translation.z, m_translateSnap);
            }

            glm::vec3 newPosition = m_startPosition + translation;
            m_positionDelta = newPosition - m_currentPosition;
            m_currentPosition = newPosition;
            break;
        }

        case GizmoMode::Rotate: {
            // For 2D, we only rotate around Z axis
            // Calculate angle from center to mouse
            glm::vec2 toMouse = worldPos - glm::vec2(m_startPosition);
            glm::vec2 toStart = startWorld - glm::vec2(m_startPosition);

            f32 currentAngle = std::atan2(toMouse.y, toMouse.x);
            f32 startAngle = std::atan2(toStart.y, toStart.x);
            f32 angleDelta = currentAngle - startAngle;

            if (m_snapEnabled) {
                f32 snapRad = glm::radians(m_rotateSnap);
                angleDelta = snapValue(angleDelta, snapRad);
            }

            glm::vec3 newRotation = m_startRotation;
            newRotation.z += angleDelta;

            m_rotationDelta = newRotation - m_currentRotation;
            m_currentRotation = newRotation;
            break;
        }

        case GizmoMode::Scale: {
            // Calculate scale based on distance from center
            glm::vec2 toMouse = worldPos - glm::vec2(m_startPosition);
            glm::vec2 toStart = startWorld - glm::vec2(m_startPosition);

            f32 currentDist = glm::length(toMouse);
            f32 startDist = glm::length(toStart);

            f32 scaleFactor = 1.0f;
            if (startDist > 0.001f) {
                scaleFactor = currentDist / startDist;
            }

            glm::vec3 scaleMultiplier(1.0f);

            switch (m_activeAxis) {
                case GizmoAxis::X:
                    scaleMultiplier.x = scaleFactor;
                    break;
                case GizmoAxis::Y:
                    scaleMultiplier.y = scaleFactor;
                    break;
                case GizmoAxis::XY:
                case GizmoAxis::XYZ:
                    scaleMultiplier = glm::vec3(scaleFactor);
                    break;
                default:
                    break;
            }

            if (m_snapEnabled) {
                scaleMultiplier.x = snapValue(scaleMultiplier.x, m_scaleSnap);
                scaleMultiplier.y = snapValue(scaleMultiplier.y, m_scaleSnap);
                scaleMultiplier.z = snapValue(scaleMultiplier.z, m_scaleSnap);
            }

            glm::vec3 newScale = m_startScale * scaleMultiplier;
            // Prevent negative or zero scale
            newScale = glm::max(newScale, glm::vec3(0.01f));

            m_scaleDelta = newScale - m_currentScale;
            m_currentScale = newScale;
            break;
        }
    }

    return true;
}

void Gizmo::end() {
    m_isManipulating = false;
    m_activeAxis = GizmoAxis::None;
}

void Gizmo::draw(const glm::vec3& position, [[maybe_unused]] const glm::vec3& rotation,
                 [[maybe_unused]] const glm::vec3& scale, f32 cameraZoom) {
    f32 size = m_gizmoSize * cameraZoom;

    switch (m_mode) {
        case GizmoMode::Translate:
            drawTranslateGizmo(position, size);
            break;
        case GizmoMode::Rotate:
            drawRotateGizmo(position, size);
            break;
        case GizmoMode::Scale:
            drawScaleGizmo(position, size);
            break;
    }
}

void Gizmo::drawTranslateGizmo(const glm::vec3& position, f32 size) {
    f32 axisLength = size * 6.0f;
    f32 thickness = size * 0.15f;
    f32 arrowSize = size * 0.4f;

    // Determine colors based on hover/active state
    glm::vec4 colorX = (m_hoveredAxis == GizmoAxis::X || m_activeAxis == GizmoAxis::X)
                           ? kAxisColorHighlight
                           : kAxisColorX;
    glm::vec4 colorY = (m_hoveredAxis == GizmoAxis::Y || m_activeAxis == GizmoAxis::Y)
                           ? kAxisColorHighlight
                           : kAxisColorY;
    glm::vec4 colorXY = (m_hoveredAxis == GizmoAxis::XY || m_activeAxis == GizmoAxis::XY)
                            ? glm::vec4(1.0f, 1.0f, 0.0f, 0.7f)
                            : kAxisColorXY;

    // X axis line
    Renderer2D::drawQuad(glm::vec3(position.x + axisLength / 2.0f, position.y, 0.5f),
                         glm::vec2(axisLength, thickness), colorX);

    // X axis arrow (triangle approximated with quad)
    Renderer2D::drawQuad(glm::vec3(position.x + axisLength + arrowSize / 2.0f, position.y, 0.5f),
                         glm::vec2(arrowSize, arrowSize * 0.6f), colorX);

    // Y axis line
    Renderer2D::drawQuad(glm::vec3(position.x, position.y + axisLength / 2.0f, 0.5f),
                         glm::vec2(thickness, axisLength), colorY);

    // Y axis arrow
    Renderer2D::drawQuad(glm::vec3(position.x, position.y + axisLength + arrowSize / 2.0f, 0.5f),
                         glm::vec2(arrowSize * 0.6f, arrowSize), colorY);

    // XY plane indicator (small square at corner)
    f32 planeSize = size * 1.5f;
    Renderer2D::drawQuad(
        glm::vec3(position.x + planeSize / 2.0f, position.y + planeSize / 2.0f, 0.4f),
        glm::vec2(planeSize, planeSize), colorXY);

    // Center point
    Renderer2D::drawQuad(position + glm::vec3(0.0f, 0.0f, 0.5f), glm::vec2(size * 0.3f), kCenterColor);
}

void Gizmo::drawRotateGizmo(const glm::vec3& position, f32 size) {
    f32 radius = size * 5.0f;
    f32 thickness = size * 0.15f;
    i32 segments = 32;

    glm::vec4 colorZ = (m_hoveredAxis == GizmoAxis::Z || m_activeAxis == GizmoAxis::Z)
                           ? kAxisColorHighlight
                           : kAxisColorZ;

    // Draw circle for Z rotation (main rotation in 2D)
    for (i32 i = 0; i < segments; ++i) {
        f32 angle1 = static_cast<f32>(i) / static_cast<f32>(segments) * glm::two_pi<f32>();
        f32 angle2 = static_cast<f32>(i + 1) / static_cast<f32>(segments) * glm::two_pi<f32>();

        glm::vec2 p1(std::cos(angle1) * radius, std::sin(angle1) * radius);
        glm::vec2 p2(std::cos(angle2) * radius, std::sin(angle2) * radius);
        glm::vec2 mid = (p1 + p2) * 0.5f;
        f32 segLength = glm::length(p2 - p1);

        // Calculate rotation angle for this segment
        f32 segAngle = std::atan2(p2.y - p1.y, p2.x - p1.x);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f),
                                             position + glm::vec3(mid, 0.5f));
        transform = glm::rotate(transform, segAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, glm::vec3(segLength, thickness, 1.0f));

        Renderer2D::drawQuad(transform, colorZ);
    }

    // Center point
    Renderer2D::drawQuad(position + glm::vec3(0.0f, 0.0f, 0.5f), glm::vec2(size * 0.3f), kCenterColor);
}

void Gizmo::drawScaleGizmo(const glm::vec3& position, f32 size) {
    f32 axisLength = size * 5.0f;
    f32 thickness = size * 0.15f;
    f32 boxSize = size * 0.4f;

    glm::vec4 colorX = (m_hoveredAxis == GizmoAxis::X || m_activeAxis == GizmoAxis::X)
                           ? kAxisColorHighlight
                           : kAxisColorX;
    glm::vec4 colorY = (m_hoveredAxis == GizmoAxis::Y || m_activeAxis == GizmoAxis::Y)
                           ? kAxisColorHighlight
                           : kAxisColorY;
    glm::vec4 colorXY = (m_hoveredAxis == GizmoAxis::XY || m_activeAxis == GizmoAxis::XY ||
                         m_hoveredAxis == GizmoAxis::XYZ || m_activeAxis == GizmoAxis::XYZ)
                            ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)
                            : kCenterColor;

    // X axis line
    Renderer2D::drawQuad(glm::vec3(position.x + axisLength / 2.0f, position.y, 0.5f),
                         glm::vec2(axisLength, thickness), colorX);

    // X axis box (scale handle)
    Renderer2D::drawQuad(glm::vec3(position.x + axisLength, position.y, 0.5f),
                         glm::vec2(boxSize, boxSize), colorX);

    // Y axis line
    Renderer2D::drawQuad(glm::vec3(position.x, position.y + axisLength / 2.0f, 0.5f),
                         glm::vec2(thickness, axisLength), colorY);

    // Y axis box
    Renderer2D::drawQuad(glm::vec3(position.x, position.y + axisLength, 0.5f),
                         glm::vec2(boxSize, boxSize), colorY);

    // Center box (uniform scale)
    Renderer2D::drawQuad(position + glm::vec3(0.0f, 0.0f, 0.5f), glm::vec2(boxSize * 1.2f), colorXY);
}

GizmoAxis Gizmo::hitTest(const glm::vec2& mousePos, const glm::vec3& position, f32 cameraZoom) {
    f32 size = m_gizmoSize * cameraZoom;
    glm::vec2 gizmoPos2D(position.x, position.y);
    glm::vec2 toMouse = mousePos - gizmoPos2D;

    f32 hitRadius = size * 0.8f;

    switch (m_mode) {
        case GizmoMode::Translate: {
            f32 axisLength = size * 6.0f;
            f32 planeSize = size * 1.5f;

            // Check XY plane first (center square)
            if (std::abs(toMouse.x) < planeSize && std::abs(toMouse.y) < planeSize &&
                toMouse.x > 0 && toMouse.y > 0) {
                m_hoveredAxis = GizmoAxis::XY;
                return GizmoAxis::XY;
            }

            // Check X axis
            if (toMouse.x > 0 && toMouse.x < axisLength + hitRadius &&
                std::abs(toMouse.y) < hitRadius) {
                m_hoveredAxis = GizmoAxis::X;
                return GizmoAxis::X;
            }

            // Check Y axis
            if (toMouse.y > 0 && toMouse.y < axisLength + hitRadius &&
                std::abs(toMouse.x) < hitRadius) {
                m_hoveredAxis = GizmoAxis::Y;
                return GizmoAxis::Y;
            }
            break;
        }

        case GizmoMode::Rotate: {
            f32 radius = size * 5.0f;
            f32 dist = glm::length(toMouse);

            // Check if near the rotation circle
            if (std::abs(dist - radius) < hitRadius * 2.0f) {
                m_hoveredAxis = GizmoAxis::Z;
                return GizmoAxis::Z;
            }
            break;
        }

        case GizmoMode::Scale: {
            f32 axisLength = size * 5.0f;
            f32 boxSize = size * 0.5f;

            // Check center (uniform scale)
            if (std::abs(toMouse.x) < boxSize && std::abs(toMouse.y) < boxSize) {
                m_hoveredAxis = GizmoAxis::XYZ;
                return GizmoAxis::XYZ;
            }

            // Check X axis handle
            if (std::abs(toMouse.x - axisLength) < boxSize && std::abs(toMouse.y) < boxSize) {
                m_hoveredAxis = GizmoAxis::X;
                return GizmoAxis::X;
            }

            // Check Y axis handle
            if (std::abs(toMouse.y - axisLength) < boxSize && std::abs(toMouse.x) < boxSize) {
                m_hoveredAxis = GizmoAxis::Y;
                return GizmoAxis::Y;
            }

            // Check X axis line
            if (toMouse.x > 0 && toMouse.x < axisLength && std::abs(toMouse.y) < hitRadius) {
                m_hoveredAxis = GizmoAxis::X;
                return GizmoAxis::X;
            }

            // Check Y axis line
            if (toMouse.y > 0 && toMouse.y < axisLength && std::abs(toMouse.x) < hitRadius) {
                m_hoveredAxis = GizmoAxis::Y;
                return GizmoAxis::Y;
            }
            break;
        }
    }

    m_hoveredAxis = GizmoAxis::None;
    return GizmoAxis::None;
}

f32 Gizmo::snapValue(f32 value, f32 snap) const {
    if (snap <= 0.0f) {
        return value;
    }
    return std::round(value / snap) * snap;
}

}  // namespace limbo::editor
