#include "ViewportPanel.hpp"

#include "../commands/EntityCommands.hpp"
#include "../commands/PropertyCommands.hpp"
#include "EditorApp.hpp"
#include "limbo/debug/Log.hpp"
#include "limbo/scene/Prefab.hpp"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <filesystem>
#include <limits>

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
    // Only handle editor camera/gizmo input when NOT in play mode
    bool const isPlaying = m_editor.getEditorState() != EditorState::Edit;

    if (!isPlaying && (m_viewportFocused || m_viewportHovered)) {
        handleCameraInput(deltaTime);
    }

    // Handle gizmo mode switching with keyboard shortcuts (only in edit mode)
    if (!isPlaying && m_viewportFocused) {
        if (Input::isKeyPressed(Key::W)) {
            m_gizmo.setMode(GizmoMode::Translate);
        }
        if (Input::isKeyPressed(Key::E)) {
            m_gizmo.setMode(GizmoMode::Rotate);
        }
        if (Input::isKeyPressed(Key::R)) {
            m_gizmo.setMode(GizmoMode::Scale);
        }
        // Toggle raycast mode with T key
        if (Input::isKeyPressed(Key::T)) {
            m_raycastMode = !m_raycastMode;
            if (!m_raycastMode) {
                m_raycastDragging = false;
                m_lastRaycastHit = RaycastHit2D{};
            }
        }
    }

    // Handle gizmo interaction and entity picking (only when not in raycast mode)
    if (m_viewportHovered && !m_raycastMode) {
        handleGizmoInput();
        handleEntityPicking();
    }

    // Handle raycast tool interaction
    if (m_viewportHovered && m_raycastMode) {
        handleRaycastTool();
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
    ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Viewport", &m_open, windowFlags);

    // Render toolbar at the top (before viewport content)
    ImGui::PopStyleVar();  // Temporarily restore padding for toolbar
    renderToolbar();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

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
                        LIMBO_LOG_EDITOR_INFO("Created sprite from: {}", assetPath);

                        m_editor.selectEntity(e);
                    });
                m_editor.executeCommand(std::move(cmd));
            } else if (ext == ".prefab") {
                // Load and instantiate prefab at drop position
                Prefab prefab;
                if (prefab.loadFromFile(path)) {
                    Entity instance =
                        prefab.instantiate(m_editor.getWorld(), glm::vec3(worldPos, 0.0f));
                    if (instance.isValid()) {
                        m_editor.selectEntity(instance);
                        m_editor.markSceneModified();
                        LIMBO_LOG_EDITOR_INFO("Instantiated prefab '{}' at ({}, {})",
                                              prefab.getName(), worldPos.x, worldPos.y);
                    } else {
                        LIMBO_LOG_EDITOR_ERROR("Failed to instantiate prefab: {}", assetPath);
                    }
                } else {
                    LIMBO_LOG_EDITOR_ERROR("Failed to load prefab: {}", assetPath);
                }
            } else {
                LIMBO_LOG_EDITOR_INFO("Dropped asset at ({}, {}): {}", worldPos.x, worldPos.y,
                                      assetPath);
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

    // Draw physics debug visualization BEFORE sprites so sprites render on top
    // Always draw from ECS components (TransformComponent) so debug shapes match
    // the interpolated sprite positions in both edit and play mode
    if (m_editor.isPhysicsDebugEnabled()) {
        drawPhysicsShapes();
        // Flush physics debug lines before drawing sprites on top
        Renderer2D::flush();
    }

    // Render all entities (sprites render on top of physics debug)
    auto& world = m_editor.getWorld();
    world.each<TransformComponent, SpriteRendererComponent>(
        [](World::EntityId, TransformComponent& transform, SpriteRendererComponent& sprite) {
            Renderer2D::drawQuad(transform.getMatrix(), sprite.color);
        });

    // Render QuadRendererComponent entities
    world.each<TransformComponent, QuadRendererComponent>(
        [](World::EntityId, TransformComponent& transform, QuadRendererComponent& quad) {
            // Build transform matrix with quad size
            glm::mat4 mat = glm::translate(glm::mat4(1.0f), transform.position);
            mat = glm::rotate(mat, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            mat = glm::rotate(mat, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            mat = glm::rotate(mat, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            mat = glm::scale(mat, glm::vec3(quad.size.x * transform.scale.x,
                                            quad.size.y * transform.scale.y, 1.0f));
            Renderer2D::drawQuad(mat, quad.color);
        });

    // Render CircleRendererComponent entities
    world.each<TransformComponent, CircleRendererComponent>(
        [](World::EntityId, TransformComponent& transform, CircleRendererComponent& circle) {
            glm::vec2 pos{transform.position.x, transform.position.y};
            f32 scale = glm::max(transform.scale.x, transform.scale.y);
            Renderer2D::drawFilledCircle(pos, circle.radius * scale, circle.color, circle.segments);
        });

    // Draw gizmos for selected entity (on top of everything)
    drawGizmos();

    // Draw raycast debug visualization
    drawRaycastDebug();

    Renderer2D::endScene();
}

void ViewportPanel::drawGrid() {
    glm::vec4 const gridColor(0.3f, 0.3f, 0.3f, 0.5f);
    glm::vec4 const axisColorX(0.8f, 0.2f, 0.2f, 0.8f);
    glm::vec4 const axisColorY(0.2f, 0.8f, 0.2f, 0.8f);

    // Smooth step: snap to nearest power-of-2 friendly sequence (1, 2, 5, 10, ...)
    // Use log10 so the step doubles/quintuples smoothly with zoom
    float const rawStep = m_gridSize * m_cameraZoom * 0.15f;
    float const logStep = std::log10(rawStep);
    float const floorLog = std::floor(logStep);
    float const base = std::pow(10.0f, floorLog);
    float const frac = rawStep / base;
    float step;
    if (frac < 1.5f) {
        step = base;
    } else if (frac < 3.5f) {
        step = base * 2.0f;
    } else if (frac < 7.5f) {
        step = base * 5.0f;
    } else {
        step = base * 10.0f;
    }
    step = glm::max(step, 0.01f);

    // Grid extent covers visible area plus margin
    float const aspect = m_viewportSize.x / m_viewportSize.y;
    float const visibleW = aspect * m_cameraZoom + step * 2.0f;
    float const visibleH = m_cameraZoom + step * 2.0f;

    // Snap grid range to step boundaries around camera position
    i32 const minX = static_cast<i32>(std::floor((m_cameraPosition.x - visibleW) / step));
    i32 const maxX = static_cast<i32>(std::ceil((m_cameraPosition.x + visibleW) / step));
    i32 const minY = static_cast<i32>(std::floor((m_cameraPosition.y - visibleH) / step));
    i32 const maxY = static_cast<i32>(std::ceil((m_cameraPosition.y + visibleH) / step));

    float const gridThickness = 0.005f * m_cameraZoom;
    float const axisThickness = 0.02f * m_cameraZoom;
    float const gridLen = visibleH * 2.0f;
    float const gridLenH = visibleW * 2.0f;

    // Draw vertical grid lines (integer multiples of step, skip origin)
    for (i32 ix = minX; ix <= maxX; ++ix) {
        float const x = static_cast<float>(ix) * step;
        if (ix == 0) {
            continue;  // axis drawn separately
        }
        Renderer2D::drawQuad(glm::vec3(x, m_cameraPosition.y, -0.1f),
                             glm::vec2(gridThickness, gridLen), gridColor);
    }

    // Draw horizontal grid lines (integer multiples of step, skip origin)
    for (i32 iy = minY; iy <= maxY; ++iy) {
        float const y = static_cast<float>(iy) * step;
        if (iy == 0) {
            continue;
        }
        Renderer2D::drawQuad(glm::vec3(m_cameraPosition.x, y, -0.1f),
                             glm::vec2(gridLenH, gridThickness), gridColor);
    }

    // Always draw axis lines at exact origin (no float accumulation)
    Renderer2D::drawQuad(glm::vec3(0.0f, m_cameraPosition.y, -0.05f),
                         glm::vec2(axisThickness, gridLen), axisColorY);
    Renderer2D::drawQuad(glm::vec3(m_cameraPosition.x, 0.0f, -0.05f),
                         glm::vec2(gridLenH, axisThickness), axisColorX);
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

    // Draw physics debug behind sprites (negative z)
    constexpr f32 debugZ = -0.5f;

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
            glm::vec3 pos{transform.position.x + box.offset.x, transform.position.y + box.offset.y,
                          debugZ};
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
            glm::vec3 pos{transform.position.x + circle.offset.x,
                          transform.position.y + circle.offset.y, debugZ};
            f32 radius = circle.radius * glm::max(transform.scale.x, transform.scale.y);

            Renderer2D::drawCircle(pos, radius, color);
        });
}

void ViewportPanel::handleEntityPicking() {
    // Only pick on left click when not manipulating gizmo
    if (!Input::isMouseButtonPressed(MouseButton::Left) || m_gizmo.isManipulating()) {
        return;
    }

    // Get mouse position in world space
    ImVec2 mousePos = ImGui::GetMousePos();
    glm::vec2 viewportMousePos(mousePos.x - m_viewportBounds[0].x,
                               mousePos.y - m_viewportBounds[0].y);
    glm::vec2 worldMousePos = screenToWorld(viewportMousePos);

    // Check if clicking on gizmo first (skip picking if on gizmo)
    Entity selectedEntity = m_editor.getSelectedEntity();
    if (selectedEntity.isValid() && selectedEntity.hasComponent<TransformComponent>()) {
        const auto& transform = selectedEntity.getComponent<TransformComponent>();
        GizmoAxis axis = m_gizmo.hitTest(worldMousePos, transform.position, m_cameraZoom);
        if (axis != GizmoAxis::None) {
            return;  // Clicking on gizmo, don't pick
        }
    }

    // Pick entity at mouse position
    Entity picked = pickEntityAt(worldMousePos);
    if (picked.isValid()) {
        m_editor.selectEntity(picked);
    } else {
        m_editor.deselectAll();
    }
}

bool ViewportPanel::hitTestQuad(const glm::vec2& worldPos, const TransformComponent& transform,
                                const glm::vec2& size) const {
    // Transform world position to local space of the quad
    glm::vec2 localPos = worldPos - glm::vec2(transform.position);

    // Apply inverse rotation (rotate point in opposite direction)
    f32 angle = -transform.rotation.z;
    f32 cosA = std::cos(angle);
    f32 sinA = std::sin(angle);
    glm::vec2 rotatedPos(localPos.x * cosA - localPos.y * sinA,
                         localPos.x * sinA + localPos.y * cosA);

    // Check bounds in local space (size is full width/height, centered at origin)
    glm::vec2 halfSize = size * glm::vec2(transform.scale) * 0.5f;
    return std::abs(rotatedPos.x) <= halfSize.x && std::abs(rotatedPos.y) <= halfSize.y;
}

bool ViewportPanel::hitTestCircle(const glm::vec2& worldPos, const TransformComponent& transform,
                                  f32 radius) const {
    // Simple distance check from center
    glm::vec2 center(transform.position.x, transform.position.y);
    f32 scaledRadius = radius * glm::max(transform.scale.x, transform.scale.y);
    f32 distSq = glm::dot(worldPos - center, worldPos - center);
    return distSq <= scaledRadius * scaledRadius;
}

Entity ViewportPanel::pickEntityAt(const glm::vec2& worldPos) const {
    auto& world = m_editor.getWorld();
    Entity bestPick;
    i32 bestLayer = std::numeric_limits<i32>::min();
    i32 bestOrder = std::numeric_limits<i32>::min();

    // Check CircleRenderers (prioritize by sorting layer/order)
    world.each<TransformComponent, CircleRendererComponent>(
        [&](World::EntityId id, TransformComponent& transform, CircleRendererComponent& circle) {
            if (hitTestCircle(worldPos, transform, circle.radius)) {
                // Higher layer/order = rendered on top = should be picked first
                if (circle.sortingLayer > bestLayer ||
                    (circle.sortingLayer == bestLayer && circle.sortingOrder > bestOrder)) {
                    bestLayer = circle.sortingLayer;
                    bestOrder = circle.sortingOrder;
                    bestPick = Entity(id, &world);
                }
            }
        });

    // Check QuadRenderers
    world.each<TransformComponent, QuadRendererComponent>(
        [&](World::EntityId id, TransformComponent& transform, QuadRendererComponent& quad) {
            if (hitTestQuad(worldPos, transform, quad.size)) {
                if (quad.sortingLayer > bestLayer ||
                    (quad.sortingLayer == bestLayer && quad.sortingOrder > bestOrder)) {
                    bestLayer = quad.sortingLayer;
                    bestOrder = quad.sortingOrder;
                    bestPick = Entity(id, &world);
                }
            }
        });

    // Check SpriteRenderers (assume 1x1 size scaled by transform)
    world.each<TransformComponent, SpriteRendererComponent>(
        [&](World::EntityId id, TransformComponent& transform, SpriteRendererComponent& sprite) {
            // Sprites are 1x1 units centered, scaled by transform
            if (hitTestQuad(worldPos, transform, glm::vec2(1.0f))) {
                if (sprite.sortingLayer > bestLayer ||
                    (sprite.sortingLayer == bestLayer && sprite.sortingOrder > bestOrder)) {
                    bestLayer = sprite.sortingLayer;
                    bestOrder = sprite.sortingOrder;
                    bestPick = Entity(id, &world);
                }
            }
        });

    return bestPick;
}

void ViewportPanel::renderToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    // Gizmo mode buttons
    bool translateMode = m_gizmo.getMode() == GizmoMode::Translate;
    bool rotateMode = m_gizmo.getMode() == GizmoMode::Rotate;
    bool scaleMode = m_gizmo.getMode() == GizmoMode::Scale;

    if (translateMode) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("W##Translate")) {
        m_gizmo.setMode(GizmoMode::Translate);
        m_raycastMode = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Translate (W)");
    }
    if (translateMode) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (rotateMode) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("E##Rotate")) {
        m_gizmo.setMode(GizmoMode::Rotate);
        m_raycastMode = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Rotate (E)");
    }
    if (rotateMode) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (scaleMode) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("R##Scale")) {
        m_gizmo.setMode(GizmoMode::Scale);
        m_raycastMode = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Scale (R)");
    }
    if (scaleMode) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Raycast debug tool
    bool const wasRaycastMode = m_raycastMode;
    if (wasRaycastMode) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.2f, 1.0f));
    }
    if (ImGui::Button("T##Raycast")) {
        m_raycastMode = !m_raycastMode;
        if (!m_raycastMode) {
            m_raycastDragging = false;
            m_lastRaycastHit = RaycastHit2D{};
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Raycast Debug Tool (T)\nClick and drag to cast a ray");
    }
    if (wasRaycastMode) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Physics debug toggle
    bool const wasPhysicsDebug = m_editor.isPhysicsDebugEnabled();
    if (wasPhysicsDebug) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Physics")) {
        m_editor.setPhysicsDebugEnabled(!wasPhysicsDebug);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Physics Debug Visualization");
    }
    if (wasPhysicsDebug) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // Grid toggle
    bool const wasShowGrid = m_showGrid;
    if (wasShowGrid) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Grid")) {
        m_showGrid = !m_showGrid;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Grid");
    }
    if (wasShowGrid) {
        ImGui::PopStyleColor();
    }

    // Show raycast hit info if available
    if (m_raycastMode && m_lastRaycastHit.hit) {
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();

        // Find entity name from hit body
        String entityName = "Unknown";
        if (m_lastRaycastHit.body) {
            auto* userData =
                reinterpret_cast<World::EntityId*>(m_lastRaycastHit.body->GetUserData().pointer);
            if (userData) {
                auto& world = m_editor.getWorld();
                Entity hitEntity(*userData, &world);
                if (hitEntity.isValid() && hitEntity.hasComponent<NameComponent>()) {
                    entityName = hitEntity.getComponent<NameComponent>().name;
                }
            }
        }

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Hit: %s (%.2f)", entityName.c_str(),
                           m_lastRaycastHit.distance);
    }

    ImGui::PopStyleVar(2);
    ImGui::Separator();
}

void ViewportPanel::handleRaycastTool() {
    ImVec2 mousePos = ImGui::GetMousePos();
    glm::vec2 viewportMousePos(mousePos.x - m_viewportBounds[0].x,
                               mousePos.y - m_viewportBounds[0].y);
    glm::vec2 worldMousePos = screenToWorld(viewportMousePos);

    if (Input::isMouseButtonPressed(MouseButton::Left)) {
        // Start new raycast
        m_raycastDragging = true;
        m_raycastStart = worldMousePos;
        m_raycastEnd = worldMousePos;
        m_lastRaycastHit = RaycastHit2D{};
    }

    if (m_raycastDragging) {
        m_raycastEnd = worldMousePos;

        if (Input::isMouseButtonDown(MouseButton::Left)) {
            // Update raycast while dragging
            glm::vec2 direction = m_raycastEnd - m_raycastStart;
            f32 distance = glm::length(direction);

            if (distance > 0.01f) {
                direction = glm::normalize(direction);

                // Only cast if physics is initialized (in play mode or with physics world)
                auto& physics = m_editor.getPhysics();
                if (physics.getWorld()) {
                    m_lastRaycastHit = physics.raycast(m_raycastStart, direction, distance, false);
                }
            }
        } else {
            // Mouse released - keep the last hit displayed
            m_raycastDragging = false;
        }
    }
}

void ViewportPanel::drawRaycastDebug() {
    if (!m_raycastMode) {
        return;
    }

    // Draw the ray line
    glm::vec2 direction = m_raycastEnd - m_raycastStart;
    f32 distance = glm::length(direction);

    if (distance < 0.01f) {
        return;
    }

    constexpr f32 debugZ = 0.5f;  // Draw on top

    // Ray line color
    glm::vec4 const rayColor{1.0f, 0.5f, 0.0f, 1.0f};     // Orange
    glm::vec4 const hitColor{1.0f, 0.0f, 0.0f, 1.0f};     // Red for hit point
    glm::vec4 const normalColor{0.0f, 1.0f, 1.0f, 1.0f};  // Cyan for normal

    // Draw ray line
    f32 const lineThickness = 0.01f * m_cameraZoom;
    glm::vec2 const midpoint = (m_raycastStart + m_raycastEnd) * 0.5f;
    f32 const angle = std::atan2(direction.y, direction.x);

    // Draw as a rotated thin quad
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(midpoint, debugZ));
    mat = glm::rotate(mat, angle, glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::scale(mat, glm::vec3(distance, lineThickness, 1.0f));
    Renderer2D::drawQuad(mat, rayColor);

    // Draw start point
    Renderer2D::drawFilledCircle(m_raycastStart, 0.03f * m_cameraZoom, rayColor, 16);

    // Draw end point or hit point
    if (m_lastRaycastHit.hit) {
        // Draw hit point
        Renderer2D::drawFilledCircle(m_lastRaycastHit.point, 0.04f * m_cameraZoom, hitColor, 16);

        // Draw normal arrow
        glm::vec2 normalEnd =
            m_lastRaycastHit.point + m_lastRaycastHit.normal * 0.2f * m_cameraZoom;
        glm::vec2 normalDir = normalEnd - m_lastRaycastHit.point;
        f32 normalLen = glm::length(normalDir);
        if (normalLen > 0.01f) {
            glm::vec2 normalMid = (m_lastRaycastHit.point + normalEnd) * 0.5f;
            f32 normalAngle = std::atan2(normalDir.y, normalDir.x);

            glm::mat4 normalMat = glm::translate(glm::mat4(1.0f), glm::vec3(normalMid, debugZ));
            normalMat = glm::rotate(normalMat, normalAngle, glm::vec3(0.0f, 0.0f, 1.0f));
            normalMat = glm::scale(normalMat, glm::vec3(normalLen, lineThickness * 0.7f, 1.0f));
            Renderer2D::drawQuad(normalMat, normalColor);

            // Arrowhead
            Renderer2D::drawFilledCircle(normalEnd, 0.02f * m_cameraZoom, normalColor, 8);
        }
    } else {
        // Draw end point (no hit)
        Renderer2D::drawFilledCircle(m_raycastEnd, 0.03f * m_cameraZoom,
                                     glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), 16);
    }
}

}  // namespace limbo::editor
