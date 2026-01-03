#pragma once

#include <limbo/Limbo.hpp>

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

private:
    void handleCameraInput(f32 deltaTime);
    void renderScene();
    void drawGrid();
    void drawGizmos();

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
};

} // namespace limbo::editor
