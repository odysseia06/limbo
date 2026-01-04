#include "ViewportPanel.hpp"
#include "EditorApp.hpp"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace limbo::editor {

ViewportPanel::ViewportPanel(EditorApp& editor) : m_editor(editor) {}

void ViewportPanel::init() {
    // Initialize camera
    float const aspect = m_viewportSize.x / m_viewportSize.y;
    m_camera = OrthographicCamera(-aspect * m_cameraZoom, aspect * m_cameraZoom, -m_cameraZoom,
                                  m_cameraZoom);
}

void ViewportPanel::shutdown() {}

void ViewportPanel::update(f32 deltaTime) {
    if (m_viewportFocused || m_viewportHovered) {
        handleCameraInput(deltaTime);
    }
}

void ViewportPanel::handleCameraInput(f32 deltaTime) {
    // Pan with middle mouse or WASD when focused
    float const panSpeed = 2.0f * m_cameraZoom * deltaTime;

    if (m_viewportFocused) {
        if (Input::isKeyDown(Key::W) || Input::isKeyDown(Key::Up)) {
            m_cameraPosition.y += panSpeed;
        }
        if (Input::isKeyDown(Key::S) || Input::isKeyDown(Key::Down)) {
            m_cameraPosition.y -= panSpeed;
        }
        if (Input::isKeyDown(Key::A) || Input::isKeyDown(Key::Left)) {
            m_cameraPosition.x -= panSpeed;
        }
        if (Input::isKeyDown(Key::D) || Input::isKeyDown(Key::Right)) {
            m_cameraPosition.x += panSpeed;
        }

        // Reset camera with Home key
        if (Input::isKeyPressed(Key::Home)) {
            m_cameraPosition = glm::vec2(0.0f);
            m_cameraZoom = 1.0f;
        }
    }

    // Zoom with scroll wheel when hovered
    if (m_viewportHovered) {
        float const scroll = Input::getScrollY();
        if (scroll != 0.0f) {
            m_cameraZoom -= scroll * 0.1f * m_cameraZoom;
            m_cameraZoom = glm::clamp(m_cameraZoom, 0.1f, 50.0f);
        }
    }

    // Update camera projection
    float const aspect = m_viewportSize.x / m_viewportSize.y;
    m_camera.setProjection(-aspect * m_cameraZoom, aspect * m_cameraZoom, -m_cameraZoom,
                           m_cameraZoom);
    m_camera.setPosition(glm::vec3(m_cameraPosition, 0.0f));
}

void ViewportPanel::render() {
    if (!m_open) {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", &m_open);

    // Get viewport info
    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();

    ImVec2 const viewportPanelSize = ImGui::GetContentRegionAvail();
    m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y};

    // Render scene to viewport
    renderScene();

    // Viewport bounds for mouse picking
    ImVec2 const minBound = ImGui::GetWindowContentRegionMin();
    ImVec2 const maxBound = ImGui::GetWindowContentRegionMax();
    ImVec2 const windowPos = ImGui::GetWindowPos();

    m_viewportBounds[0] = {minBound.x + windowPos.x, minBound.y + windowPos.y};
    m_viewportBounds[1] = {maxBound.x + windowPos.x, maxBound.y + windowPos.y};

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::renderScene() {
    // For now, render directly to the screen
    // TODO: Render to framebuffer and display as image

    // Begin scene rendering
    Renderer2D::beginScene(m_camera);

    // Draw grid
    if (m_showGrid) {
        drawGrid();
    }

    // Render all entities
    auto& world = m_editor.getWorld();
    world.each<TransformComponent, SpriteRendererComponent>(
        [](World::EntityId, TransformComponent& transform, SpriteRendererComponent& sprite) {
            Renderer2D::drawQuad(transform.getMatrix(), sprite.color);
        });

    // Draw gizmos for selected entity
    drawGizmos();

    Renderer2D::endScene();
}

void ViewportPanel::drawGrid() {
    // Draw a simple grid
    glm::vec4 const gridColor(0.3f, 0.3f, 0.3f, 0.5f);
    glm::vec4 const axisColorX(0.8f, 0.2f, 0.2f, 0.8f);
    glm::vec4 const axisColorY(0.2f, 0.8f, 0.2f, 0.8f);

    float const gridExtent = 20.0f;
    float step = m_gridSize;

    // Adjust step based on zoom
    if (m_cameraZoom > 5.0f) {
        step = 2.0f;
    }
    if (m_cameraZoom > 10.0f) {
        step = 5.0f;
    }
    if (m_cameraZoom < 0.5f) {
        step = 0.5f;
    }
    if (m_cameraZoom < 0.2f) {
        step = 0.1f;
    }

    // Draw vertical lines
    for (float x = -gridExtent; x <= gridExtent; x += step) {
        glm::vec4 const color = (std::abs(x) < 0.001f) ? axisColorY : gridColor;
        float const thickness = (std::abs(x) < 0.001f) ? 0.02f : 0.005f;

        Renderer2D::drawQuad(glm::vec3(x, 0.0f, -0.1f), glm::vec2(thickness, gridExtent * 2.0f),
                             color);
    }

    // Draw horizontal lines
    for (float y = -gridExtent; y <= gridExtent; y += step) {
        glm::vec4 const color = (std::abs(y) < 0.001f) ? axisColorX : gridColor;
        float const thickness = (std::abs(y) < 0.001f) ? 0.02f : 0.005f;

        Renderer2D::drawQuad(glm::vec3(0.0f, y, -0.1f), glm::vec2(gridExtent * 2.0f, thickness),
                             color);
    }
}

void ViewportPanel::drawGizmos() {
    // Get selected entity from editor
    // TODO: Draw translation/rotation/scale gizmos

    // For now, just draw a selection outline
    // This would be implemented with the editor's selected entity
}

}  // namespace limbo::editor
