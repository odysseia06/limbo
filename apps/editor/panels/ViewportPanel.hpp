#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/graphics/Framebuffer.hpp>
#include <limbo/physics/2d/Physics2D.hpp>
#include "../gizmos/Gizmo.hpp"

#include <memory>

namespace limbo::editor {

class EditorApp;

/**
 * ViewportPanel - Renders the scene and provides camera controls
 */
class ViewportPanel {
public:
    explicit ViewportPanel(EditorApp& editor);

    void init();
    void shutdown();
    void update(f32 deltaTime);
    void render();

    [[nodiscard]] bool& isOpen() { return m_open; }
    [[nodiscard]] OrthographicCamera& getCamera() { return m_camera; }

    // Gizmo access
    [[nodiscard]] Gizmo& getGizmo() { return m_gizmo; }
    [[nodiscard]] GizmoMode getGizmoMode() const { return m_gizmo.getMode(); }
    void setGizmoMode(GizmoMode mode) { m_gizmo.setMode(mode); }

private:
    void handleCameraInput(f32 deltaTime);
    void handleGizmoInput();
    void handleEntityPicking();
    void handleAssetDrop();
    void handleRaycastTool();
    void renderScene();
    void renderToolbar();
    void drawGrid();
    void drawGizmos();
    void drawPhysicsShapes();
    void drawRaycastDebug();

    [[nodiscard]] glm::vec2 screenToWorld(const glm::vec2& screenPos) const;

    // Shape-aware picking helpers
    [[nodiscard]] bool hitTestQuad(const glm::vec2& worldPos, const TransformComponent& transform,
                                   const glm::vec2& size) const;
    [[nodiscard]] bool hitTestCircle(const glm::vec2& worldPos, const TransformComponent& transform,
                                     f32 radius) const;
    [[nodiscard]] Entity pickEntityAt(const glm::vec2& worldPos) const;

private:
    EditorApp& m_editor;
    bool m_open = true;

    // Camera
    OrthographicCamera m_camera;
    f32 m_cameraZoom = 1.0f;
    glm::vec2 m_cameraPosition{0.0f};

    // Viewport
    glm::vec2 m_viewportSize{1280.0f, 720.0f};
    glm::vec2 m_viewportBounds[2];
    bool m_viewportFocused = false;
    bool m_viewportHovered = false;

    // Grid
    bool m_showGrid = true;
    f32 m_gridSize = 1.0f;

    // Raycast debug tool
    bool m_raycastMode = false;
    bool m_raycastDragging = false;
    glm::vec2 m_raycastStart{0.0f};
    glm::vec2 m_raycastEnd{0.0f};
    RaycastHit2D m_lastRaycastHit;

    // Gizmo
    Gizmo m_gizmo;
    bool m_gizmoWasManipulating = false;
    TransformComponent m_gizmoStartTransform;

    // Framebuffer for offscreen rendering
    std::unique_ptr<Framebuffer> m_framebuffer;
};

}  // namespace limbo::editor
