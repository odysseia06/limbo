#include "ViewportPanel.hpp"
#include "EditorApp.hpp"
#include "../commands/EntityCommands.hpp"
#include "../commands/PropertyCommands.hpp"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

namespace limbo::editor {

ViewportPanel::ViewportPanel(EditorApp& editor) : m_editor(editor) {}

void ViewportPanel::init() {
    // Initialize camera
    float const aspect = m_viewportSize.x / m_viewportSize.y;
    m_camera = OrthographicCamera(-aspect * m_cameraZoom, aspect * m_cameraZoom, -m_cameraZoom,
                                  m_cameraZoom);

    // Initialize framebuffer
    FramebufferSpec spec;
    spec.width = static_cast<u32>(m_viewportSize.x);
    spec.height = static_cast<u32>(m_viewportSize.y);
    m_framebuffer = std::make_unique<Framebuffer>(spec);
}

void ViewportPanel::shutdown() {}

void ViewportPanel::update(f32 deltaTime) {
    if (m_viewportFocused || m_viewportHovered) {
        handleCameraInput(deltaTime);
    }

    // Handle gizmo mode switching with keyboard shortcuts
    if (m_viewportFocused) {
        if (Input::isKeyPressed(Key::W)) {
            m_gizmo.setMode(GizmoMode::Translate);
        }
        if (Input::isKeyPressed(Key::E)) {
            m_gizmo.setMode(GizmoMode::Rotate);
        }
        if (Input::isKeyPressed(Key::R)) {
            m_gizmo.setMode(GizmoMode::Scale);
        }
    }

    // Handle gizmo interaction
    if (m_viewportHovered) {
        handleGizmoInput();
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

    // Resize framebuffer if viewport size changed
    if (m_framebuffer &&
        (viewportPanelSize.x != m_viewportSize.x || viewportPanelSize.y != m_viewportSize.y)) {
        m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y};
        if (m_viewportSize.x > 0 && m_viewportSize.y > 0) {
            m_framebuffer->resize(static_cast<u32>(m_viewportSize.x),
                                  static_cast<u32>(m_viewportSize.y));
        }
    } else {
        m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y};
    }

    // Render scene to framebuffer
    if (m_framebuffer && m_viewportSize.x > 0 && m_viewportSize.y > 0) {
        m_framebuffer->bind();
        m_framebuffer->clear(0.1f, 0.1f, 0.1f, 1.0f);

        renderScene();

        m_framebuffer->unbind();

        // Display the framebuffer texture in ImGui
        u64 textureId = m_framebuffer->getColorAttachmentId();
        ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(textureId)),
                     ImVec2(m_viewportSize.x, m_viewportSize.y), ImVec2(0, 1),
                     ImVec2(1, 0));  // Flip Y for OpenGL
    }

    // Viewport bounds for mouse picking
    ImVec2 const minBound = ImGui::GetWindowContentRegionMin();
    ImVec2 const maxBound = ImGui::GetWindowContentRegionMax();
    ImVec2 const windowPos = ImGui::GetWindowPos();

    m_viewportBounds[0] = {minBound.x + windowPos.x, minBound.y + windowPos.y};
    m_viewportBounds[1] = {maxBound.x + windowPos.x, maxBound.y + windowPos.y};

    // Handle drag-drop from asset browser
    handleAssetDrop();

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::handleAssetDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            const char* assetPath = static_cast<const char*>(payload->Data);
            std::filesystem::path path(assetPath);

            // Get mouse position and convert to world coordinates
            ImVec2 mousePos = ImGui::GetMousePos();
            glm::vec2 viewportMousePos(mousePos.x - m_viewportBounds[0].x,
                                       mousePos.y - m_viewportBounds[0].y);
            glm::vec2 worldPos = screenToWorld(viewportMousePos);

            // Handle different asset types
            String const ext = path.extension().string();
            String const filename = path.stem().string();

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
                ext == ".tga") {
                // Create sprite entity at drop position
                auto cmd = std::make_unique<CreateEntityCommand>(
                    m_editor.getWorld(), filename, [this, worldPos, assetPath](Entity e) {
                        // Set position
                        auto& transform = e.getComponent<TransformComponent>();
                        transform.position = glm::vec3(worldPos, 0.0f);

                        // Add sprite component
                        e.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));

                        // TODO: Load texture and assign to sprite
                        spdlog::info("Created sprite from: {}", assetPath);

                        m_editor.selectEntity(e);
                    });
                m_editor.executeCommand(std::move(cmd));
            } else if (ext == ".prefab" || ext == ".json") {
                // Try to instantiate as prefab
                spdlog::info("Dropped prefab/scene at ({}, {}): {}", worldPos.x, worldPos.y,
                             assetPath);
                // TODO: Load and instantiate prefab at position
            } else {
                spdlog::info("Dropped asset at ({}, {}): {}", worldPos.x, worldPos.y, assetPath);
            }
        }
        ImGui::EndDragDropTarget();
    }
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

    // Draw physics debug visualization
    // In Play mode: draws actual Box2D world state
    // In Edit mode: draws collider shapes from components (for scene setup)
    if (m_editor.isPhysicsDebugEnabled()) {
        if (m_editor.getEditorState() == EditorState::Play) {
            // Draw from actual physics world
            m_editor.getPhysicsDebug().draw(m_editor.getPhysics());
        } else {
            // Draw from components in edit mode
            drawPhysicsShapes();
        }
    }

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
    Entity selectedEntity = m_editor.getSelectedEntity();
    if (!selectedEntity.isValid()) {
        return;
    }

    if (!selectedEntity.hasComponent<TransformComponent>()) {
        return;
    }

    const auto& transform = selectedEntity.getComponent<TransformComponent>();

    // Draw selection outline
    glm::vec4 outlineColor(1.0f, 0.6f, 0.0f, 0.8f);
    f32 outlineThickness = 0.02f * m_cameraZoom;
    glm::vec3 pos = transform.position;
    glm::vec3 scale = transform.scale;

    // Top
    Renderer2D::drawQuad(glm::vec3(pos.x, pos.y + scale.y / 2.0f, 0.3f),
                         glm::vec2(scale.x + outlineThickness * 2, outlineThickness), outlineColor);
    // Bottom
    Renderer2D::drawQuad(glm::vec3(pos.x, pos.y - scale.y / 2.0f, 0.3f),
                         glm::vec2(scale.x + outlineThickness * 2, outlineThickness), outlineColor);
    // Left
    Renderer2D::drawQuad(glm::vec3(pos.x - scale.x / 2.0f, pos.y, 0.3f),
                         glm::vec2(outlineThickness, scale.y), outlineColor);
    // Right
    Renderer2D::drawQuad(glm::vec3(pos.x + scale.x / 2.0f, pos.y, 0.3f),
                         glm::vec2(outlineThickness, scale.y), outlineColor);

    // Draw the gizmo
    m_gizmo.draw(transform.position, transform.rotation, transform.scale, m_cameraZoom);
}

void ViewportPanel::handleGizmoInput() {
    Entity selectedEntity = m_editor.getSelectedEntity();
    if (!selectedEntity.isValid() || !selectedEntity.hasComponent<TransformComponent>()) {
        if (m_gizmo.isManipulating()) {
            m_gizmo.end();
        }
        return;
    }

    auto& transform = selectedEntity.getComponent<TransformComponent>();

    // Get mouse position relative to viewport
    ImVec2 mousePos = ImGui::GetMousePos();
    glm::vec2 viewportMousePos(mousePos.x - m_viewportBounds[0].x,
                               mousePos.y - m_viewportBounds[0].y);

    // Convert to world space for hit testing
    glm::vec2 worldMousePos = screenToWorld(viewportMousePos);

    // Check for snapping toggle (hold Ctrl for snap)
    bool snapEnabled = Input::isKeyDown(Key::LeftControl) || Input::isKeyDown(Key::RightControl);
    m_gizmo.setSnapEnabled(snapEnabled);

    if (Input::isMouseButtonPressed(MouseButton::Left)) {
        // Check if clicking on gizmo
        GizmoAxis axis = m_gizmo.hitTest(worldMousePos, transform.position, m_cameraZoom);

        if (axis != GizmoAxis::None) {
            // Start manipulation
            m_gizmo.begin(transform.position, transform.rotation, transform.scale,
                          viewportMousePos);
            m_gizmoStartTransform = transform;
            m_gizmoWasManipulating = true;
        }
    }

    if (m_gizmo.isManipulating()) {
        if (Input::isMouseButtonDown(MouseButton::Left)) {
            // Update gizmo
            m_gizmo.update(viewportMousePos, m_viewportSize, m_camera);

            // Apply transform changes
            switch (m_gizmo.getMode()) {
            case GizmoMode::Translate:
                transform.position = m_gizmo.getCurrentPosition();
                break;
            case GizmoMode::Rotate:
                transform.rotation = m_gizmo.getCurrentRotation();
                break;
            case GizmoMode::Scale:
                transform.scale = m_gizmo.getCurrentScale();
                break;
            }
        } else {
            // Mouse released - end manipulation and create undo command
            m_gizmo.end();

            if (m_gizmoWasManipulating) {
                // Create command for undo/redo
                TransformComponent newTransform = transform;
                transform = m_gizmoStartTransform;  // Restore original for command

                auto cmd = std::make_unique<SetTransformCommand>(m_editor.getWorld(),
                                                                 selectedEntity.id(), newTransform);
                m_editor.executeCommand(std::move(cmd));

                m_gizmoWasManipulating = false;
            }
        }
    }

    // Update hovered axis for visual feedback
    if (!m_gizmo.isManipulating()) {
        m_gizmo.hitTest(worldMousePos, transform.position, m_cameraZoom);
    }
}

glm::vec2 ViewportPanel::screenToWorld(const glm::vec2& screenPos) const {
    // Convert screen position to normalized device coordinates
    glm::vec2 normalizedPos = screenPos / m_viewportSize * 2.0f - 1.0f;
    normalizedPos.y = -normalizedPos.y;  // Flip Y

    // Transform to world space
    glm::mat4 invViewProj = glm::inverse(m_camera.getViewProjectionMatrix());
    glm::vec4 worldPos4 = invViewProj * glm::vec4(normalizedPos, 0.0f, 1.0f);

    return glm::vec2(worldPos4.x, worldPos4.y);
}

void ViewportPanel::drawPhysicsShapes() {
    // Draw physics collider shapes from ECS components (for edit mode visualization)
    auto& world = m_editor.getWorld();

    // Colors for different states
    glm::vec4 const staticColor{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 const kinematicColor{0.5f, 0.5f, 0.9f, 1.0f};
    glm::vec4 const dynamicColor{0.0f, 1.0f, 0.0f, 1.0f};
    glm::vec4 const triggerColor{1.0f, 1.0f, 0.0f, 0.7f};

    // Draw box colliders
    world.each<TransformComponent, BoxCollider2DComponent>(
        [&](World::EntityId id, TransformComponent& transform, BoxCollider2DComponent& box) {
            // Determine color based on body type
            glm::vec4 color = dynamicColor;
            if (world.hasComponent<Rigidbody2DComponent>(id)) {
                auto& rb = world.getComponent<Rigidbody2DComponent>(id);
                switch (rb.type) {
                case BodyType::Static:
                    color = staticColor;
                    break;
                case BodyType::Kinematic:
                    color = kinematicColor;
                    break;
                case BodyType::Dynamic:
                    color = dynamicColor;
                    break;
                }
            }
            if (box.isTrigger) {
                color = triggerColor;
            }

            // Calculate world position with offset
            glm::vec2 pos{transform.position.x + box.offset.x, transform.position.y + box.offset.y};
            glm::vec2 size{box.size.x * 2.0f * transform.scale.x,
                           box.size.y * 2.0f * transform.scale.y};

            Renderer2D::drawRect(pos, size, transform.rotation.z, color);
        });

    // Draw circle colliders
    world.each<TransformComponent, CircleCollider2DComponent>(
        [&](World::EntityId id, TransformComponent& transform, CircleCollider2DComponent& circle) {
            // Determine color based on body type
            glm::vec4 color = dynamicColor;
            if (world.hasComponent<Rigidbody2DComponent>(id)) {
                auto& rb = world.getComponent<Rigidbody2DComponent>(id);
                switch (rb.type) {
                case BodyType::Static:
                    color = staticColor;
                    break;
                case BodyType::Kinematic:
                    color = kinematicColor;
                    break;
                case BodyType::Dynamic:
                    color = dynamicColor;
                    break;
                }
            }
            if (circle.isTrigger) {
                color = triggerColor;
            }

            // Calculate world position with offset
            glm::vec2 pos{transform.position.x + circle.offset.x,
                          transform.position.y + circle.offset.y};
            f32 radius = circle.radius * glm::max(transform.scale.x, transform.scale.y);

            Renderer2D::drawCircle(pos, radius, color);
        });
}

}  // namespace limbo::editor
