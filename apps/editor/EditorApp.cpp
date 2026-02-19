#include "EditorApp.hpp"

#include "commands/EntityCommands.hpp"
#include "commands/PropertyCommands.hpp"
#include "limbo/debug/Log.hpp"
#include "limbo/imgui/DebugPanels.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace limbo::editor {

EditorApp::EditorApp()
    : m_hierarchyPanel(*this), m_inspectorPanel(*this), m_viewportPanel(*this),
      m_assetBrowserPanel(*this), m_assetPipelinePanel(*this), m_consolePanel(*this),
      m_prefabOverridesPanel(*this), m_scriptDebugPanel(*this), m_prefabStage(*this) {}

void EditorApp::onInit() {
    LIMBO_LOG_EDITOR_INFO("Limbo Editor initialized");

    // Create render context
    m_renderContext = RenderContext::create();
    if (!m_renderContext->init(getWindow())) {
        LIMBO_LOG_EDITOR_CRITICAL("Failed to initialize render context");
        requestExit();
        return;
    }

    // Initialize Renderer2D
    Renderer2D::init();

    // Initialize ImGui without layout persistence (we use programmatic layout)
    if (!m_imguiLayer.init(getWindow().getNativeHandle(), nullptr)) {
        LIMBO_LOG_EDITOR_ERROR("Failed to initialize ImGui");
    }

    // Initialize camera
    float const aspect =
        static_cast<float>(getWindow().getWidth()) / static_cast<float>(getWindow().getHeight());
    m_editorCamera = OrthographicCamera(-aspect * m_cameraZoom, aspect * m_cameraZoom,
                                        -m_cameraZoom, m_cameraZoom);

    // Initialize physics (for play mode)
    m_physics.init({0.0f, -9.81f});
    m_physicsSystem = std::make_unique<PhysicsSystem2D>(m_physics);

    // Initialize scripting (for play mode)
    m_scriptEngine.init();
    m_scriptSystem = std::make_unique<ScriptSystem>(m_scriptEngine);

    // Setup default asset path
    std::filesystem::path const assetsPath = std::filesystem::current_path() / "assets";
    if (std::filesystem::exists(assetsPath)) {
        m_assetManager.setAssetRoot(assetsPath);
    }

    // Initialize panels
    m_hierarchyPanel.init();
    m_inspectorPanel.init();
    m_viewportPanel.init();
    m_assetBrowserPanel.init();
    m_assetPipelinePanel.init();
    m_consolePanel.init();
    m_prefabOverridesPanel.init();

    // Start with a new scene
    newScene();

    LIMBO_LOG_EDITOR_INFO("Editor ready");
}

void EditorApp::onUpdate(f32 deltaTime) {
    m_deltaTime = deltaTime;

    // Handle global shortcuts
    if (Input::isKeyDown(Key::LeftControl) || Input::isKeyDown(Key::RightControl)) {
        if (Input::isKeyPressed(Key::N)) {
            newScene();
        }
        if (Input::isKeyPressed(Key::O)) {
            openScene();
        }
        if (Input::isKeyPressed(Key::S)) {
            if (Input::isKeyDown(Key::LeftShift) || Input::isKeyDown(Key::RightShift)) {
                saveSceneAs();
            } else {
                saveScene();
            }
        }
        if (Input::isKeyPressed(Key::Z)) {
            if (Input::isKeyDown(Key::LeftShift) || Input::isKeyDown(Key::RightShift)) {
                redo();
            } else {
                undo();
            }
        }
        if (Input::isKeyPressed(Key::Y)) {
            redo();
        }
    }

    // Update based on editor state
    if (m_editorState == EditorState::Play) {
        // Run script system (handles onStart, onUpdate)
        m_scriptSystem->update(getWorld(), deltaTime);

        // Run physics system
        m_physicsSystem->update(getWorld(), deltaTime);
    }

    // Update panels
    m_viewportPanel.update(deltaTime);
    m_assetPipelinePanel.update(deltaTime);

    // Reset renderer stats
    Renderer2D::resetStats();
}

void EditorApp::onRender([[maybe_unused]] f32 interpolationAlpha) {
    // Clear to dark gray
    m_renderContext->clear(0.15f, 0.15f, 0.15f, 1.0f);

    // Begin ImGui frame
    m_imguiLayer.beginFrame();

    // Render dockspace and panels
    renderDockspace();

    // Demo window (F1)
    if (Input::isKeyPressed(Key::F1)) {
        m_showDemoWindow = !m_showDemoWindow;
    }
    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    // Profiler panel
    if (m_showProfiler) {
        DebugPanels::showProfilerPanel();
    }

    // Scene select popup
    if (m_showSceneSelectPopup) {
        ImGui::OpenPopup("Open Scene");
        m_showSceneSelectPopup = false;
    }

    if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select a scene to open:");
        ImGui::Separator();

        // List available scenes
        std::filesystem::path const scenesDir = m_assetManager.getAssetRoot() / "scenes";
        if (std::filesystem::exists(scenesDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(scenesDir)) {
                if (entry.path().extension() == ".json") {
                    std::string const filename = entry.path().filename().string();
                    if (ImGui::Selectable(filename.c_str())) {
                        loadSceneFromPath(entry.path());
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        } else {
            ImGui::TextDisabled("No scenes directory found");
        }

        ImGui::Separator();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // End ImGui frame
    m_imguiLayer.endFrame();
}

void EditorApp::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                newScene();
            }
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                openScene();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                saveScene();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                saveSceneAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                requestExit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            String undoLabel = "Undo";
            if (m_commandHistory.canUndo()) {
                undoLabel += " " + m_commandHistory.getUndoDescription();
            }
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, m_commandHistory.canUndo())) {
                undo();
            }

            String redoLabel = "Redo";
            if (m_commandHistory.canRedo()) {
                redoLabel += " " + m_commandHistory.getRedoDescription();
            }
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, m_commandHistory.canRedo())) {
                redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Delete", false, m_selectedEntity.isValid())) {
                if (m_selectedEntity.isValid()) {
                    auto cmd =
                        std::make_unique<DeleteEntityCommand>(getWorld(), m_selectedEntity.id());
                    executeCommand(std::move(cmd));
                    deselectAll();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Create")) {
            // 2D Object submenu (visual only)
            if (ImGui::BeginMenu("2D Object")) {
                if (ImGui::MenuItem("Sprite")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Sprite", [this](Entity e) {
                            e.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                if (ImGui::MenuItem("Rectangle")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Rectangle", [this](Entity e) {
                            e.addComponent<QuadRendererComponent>(glm::vec4(1.0f), glm::vec2(1.0f));
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                if (ImGui::MenuItem("Circle")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Circle", [this](Entity e) {
                            e.addComponent<CircleRendererComponent>(glm::vec4(1.0f), 0.5f);
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                ImGui::EndMenu();
            }

            // 2D Physics submenu (visual + collider + rigidbody)
            if (ImGui::BeginMenu("2D Physics")) {
                if (ImGui::MenuItem("Static Rectangle")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Static Rectangle", [this](Entity e) {
                            e.addComponent<QuadRendererComponent>(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                                                                  glm::vec2(1.0f));
                            e.addComponent<BoxCollider2DComponent>(glm::vec2(0.5f));
                            e.addComponent<Rigidbody2DComponent>(BodyType::Static);
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                if (ImGui::MenuItem("Dynamic Rectangle")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Dynamic Rectangle", [this](Entity e) {
                            e.addComponent<QuadRendererComponent>(glm::vec4(1.0f), glm::vec2(1.0f));
                            e.addComponent<BoxCollider2DComponent>(glm::vec2(0.5f));
                            e.addComponent<Rigidbody2DComponent>(BodyType::Dynamic);
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                if (ImGui::MenuItem("Static Circle")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Static Circle", [this](Entity e) {
                            e.addComponent<CircleRendererComponent>(
                                glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), 0.5f);
                            e.addComponent<CircleCollider2DComponent>(0.5f);
                            e.addComponent<Rigidbody2DComponent>(BodyType::Static);
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                if (ImGui::MenuItem("Dynamic Circle")) {
                    auto cmd = std::make_unique<CreateEntityCommand>(
                        getWorld(), "Dynamic Circle", [this](Entity e) {
                            e.addComponent<CircleRendererComponent>(glm::vec4(1.0f), 0.5f);
                            e.addComponent<CircleCollider2DComponent>(0.5f);
                            e.addComponent<Rigidbody2DComponent>(BodyType::Dynamic);
                            selectEntity(e);
                        });
                    executeCommand(std::move(cmd));
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Empty Entity")) {
                auto cmd = std::make_unique<CreateEntityCommand>(
                    getWorld(), "New Entity", [this](Entity e) { selectEntity(e); });
                executeCommand(std::move(cmd));
            }
            if (ImGui::MenuItem("Camera")) {
                auto cmd =
                    std::make_unique<CreateEntityCommand>(getWorld(), "Camera", [this](Entity e) {
                        e.addComponent<CameraComponent>();
                        selectEntity(e);
                    });
                executeCommand(std::move(cmd));
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_hierarchyPanel.isOpen());
            ImGui::MenuItem("Inspector", nullptr, &m_inspectorPanel.isOpen());
            ImGui::MenuItem("Viewport", nullptr, &m_viewportPanel.isOpen());
            ImGui::MenuItem("Asset Browser", nullptr, &m_assetBrowserPanel.isOpen());
            ImGui::MenuItem("Asset Pipeline", nullptr, &m_assetPipelinePanel.isOpen());
            ImGui::MenuItem("Console", nullptr, &m_consolePanel.isOpen());
            ImGui::MenuItem("Prefab Overrides", nullptr, &m_prefabOverridesPanel.isOpen());
            ImGui::MenuItem("Script Debug", nullptr, &m_scriptDebugPanel.isOpen());
            ImGui::Separator();
            ImGui::MenuItem("Physics Debug", nullptr, &m_showPhysicsDebug);
            ImGui::MenuItem("Profiler", nullptr, &m_showProfiler);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", "F1", &m_showDemoWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Limbo Editor")) {
                // TODO: Show about dialog
            }
            ImGui::EndMenu();
        }

        // Separator between menus and toolbar
        ImGui::Separator();

        // Integrated toolbar - Play/Pause/Stop buttons
        bool const isPlaying = (m_editorState == EditorState::Play);
        bool const isPaused = (m_editorState == EditorState::Pause);

        // Play button
        if (isPlaying) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        }
        if (ImGui::Button(isPlaying ? "Playing" : "Play")) {
            if (m_editorState == EditorState::Edit) {
                onPlay();
            }
        }
        if (isPlaying) {
            ImGui::PopStyleColor();
        }

        // Pause button
        if (isPaused) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
        }
        ImGui::BeginDisabled(m_editorState == EditorState::Edit);
        if (ImGui::Button(isPaused ? "Paused" : "Pause")) {
            onPause();
        }
        ImGui::EndDisabled();
        if (isPaused) {
            ImGui::PopStyleColor();
        }

        // Stop button
        ImGui::BeginDisabled(m_editorState == EditorState::Edit);
        if (ImGui::Button("Stop")) {
            onStop();
        }
        ImGui::EndDisabled();

        ImGui::Separator();

        // Breadcrumb navigation (Unity-style)
        // Shows: Scene > Prefab (when editing prefab)
        String sceneName =
            m_currentScenePath.empty() ? "Untitled" : m_currentScenePath.filename().string();

        if (m_prefabStage.isOpen()) {
            // Scene name (clickable to return to scene)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            if (ImGui::SmallButton(sceneName.c_str())) {
                if (m_prefabStage.hasUnsavedChanges()) {
                    m_showPrefabCloseDialog = true;
                } else {
                    m_prefabStage.close(false);
                }
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Return to scene");
            }

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ">");
            ImGui::SameLine();

            // Prefab name (current location, highlighted)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 1.0f, 1.0f));
            ImGui::Text("%s", m_prefabStage.getPrefabName().c_str());
            ImGui::PopStyleColor();

            // Unsaved indicator
            if (m_prefabStage.hasUnsavedChanges()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "*");
            }

            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            // Action buttons
            if (ImGui::Button("Save")) {
                m_prefabStage.save();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Save changes to prefab asset");
            }

            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                if (m_prefabStage.hasUnsavedChanges()) {
                    m_showPrefabCloseDialog = true;
                } else {
                    m_prefabStage.close(false);
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Close prefab and return to scene");
            }
        } else {
            // Normal scene mode - just show scene name
            if (m_sceneModified) {
                sceneName += "*";
            }
            ImGui::Text("Scene: %s", sceneName.c_str());
        }

        // Right-aligned status info
        float statusWidth = 300.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - statusWidth);
        ImGui::Text("FPS: %.0f | Entities: %zu | Quads: %u",
                    static_cast<double>(1.0f / m_deltaTime), getWorld().entityCount(),
                    Renderer2D::getStats().quadCount);

        ImGui::EndMenuBar();
    }

    // Prefab close confirmation dialog
    if (m_showPrefabCloseDialog) {
        ImGui::OpenPopup("Close Prefab?");
        m_showPrefabCloseDialog = false;
    }

    if (ImGui::BeginPopupModal("Close Prefab?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Prefab '%s' has unsaved changes.", m_prefabStage.getPrefabName().c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(100, 0))) {
            m_prefabStage.close(true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(100, 0))) {
            m_prefabStage.close(false);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorApp::setupDockingLayout(ImGuiID dockspaceId) {
    // Clear any existing layout
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    // Split the dockspace into main areas
    // First split: top (75%) and bottom (25%)
    ImGuiID dockTop = 0;
    ImGuiID dockBottom = 0;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, &dockBottom, &dockTop);

    // Split top section: left (15%), center+right
    ImGuiID dockLeft = 0;
    ImGuiID dockCenterRight = 0;
    ImGui::DockBuilderSplitNode(dockTop, ImGuiDir_Left, 0.15f, &dockLeft, &dockCenterRight);

    // Split center+right: center (70% of remaining), right (30% of remaining = ~25% total)
    ImGuiID dockCenter = 0;
    ImGuiID dockRight = 0;
    ImGui::DockBuilderSplitNode(dockCenterRight, ImGuiDir_Right, 0.30f, &dockRight, &dockCenter);

    // Dock panels to their designated areas
    ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
    ImGui::DockBuilderDockWindow("Viewport", dockCenter);
    ImGui::DockBuilderDockWindow("Inspector", dockRight);
    ImGui::DockBuilderDockWindow("Prefab Overrides", dockRight);

    // Bottom section: tabbed panels (Asset Browser, Asset Pipeline, Console)
    ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
    ImGui::DockBuilderDockWindow("Asset Pipeline", dockBottom);
    ImGui::DockBuilderDockWindow("Console", dockBottom);

    // Toolbar and Status Bar are special - they don't go in the dockspace
    // They'll be rendered separately

    // Finalize the layout
    ImGui::DockBuilderFinish(dockspaceId);

    LIMBO_LOG_EDITOR_INFO("Editor layout initialized");
}

void EditorApp::renderDockspace() {
    // Setup dockspace
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // Menu bar
    renderMenuBar();

    // Dockspace with locked layout
    m_dockspaceId = ImGui::GetID("EditorDockspace");

    // Initialize the layout on first frame
    if (!m_layoutInitialized) {
        setupDockingLayout(m_dockspaceId);
        m_layoutInitialized = true;
    }

    // Create dockspace with flags to lock the layout
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
    dockspaceFlags |= ImGuiDockNodeFlags_NoUndocking;         // Prevent undocking windows
    dockspaceFlags |= ImGuiDockNodeFlags_NoDockingSplit;      // Prevent splitting dock nodes
    dockspaceFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;  // Hide the window list menu

    ImGui::DockSpace(m_dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

    ImGui::End();

    // Render panels
    m_hierarchyPanel.render();
    m_inspectorPanel.render();
    m_viewportPanel.render();
    m_assetBrowserPanel.render();
    m_assetPipelinePanel.render();
    m_consolePanel.render();
    m_prefabOverridesPanel.render();
    m_scriptDebugPanel.render(getWorld(), m_scriptSystem.get());
}

void EditorApp::newScene() {
    getWorld().clear();
    m_currentScenePath.clear();
    m_sceneModified = false;
    m_commandHistory.clear();
    deselectAll();
    LIMBO_LOG_EDITOR_INFO("New scene created");
}

bool EditorApp::executeCommand(Unique<Command> command) {
    if (m_commandHistory.execute(std::move(command))) {
        markSceneModified();
        return true;
    }
    return false;
}

void EditorApp::undo() {
    if (m_commandHistory.undo()) {
        markSceneModified();
        LIMBO_LOG_EDITOR_DEBUG("Undo: {}", m_commandHistory.getRedoDescription());
    }
}

void EditorApp::redo() {
    if (m_commandHistory.redo()) {
        markSceneModified();
        LIMBO_LOG_EDITOR_DEBUG("Redo: {}", m_commandHistory.getUndoDescription());
    }
}

void EditorApp::openScene() {
    // Show scene selection popup
    m_showSceneSelectPopup = true;
}

void EditorApp::loadSceneFromPath(const std::filesystem::path& scenePath) {
    if (std::filesystem::exists(scenePath)) {
        SceneSerializer serializer(getWorld());
        if (serializer.loadFromFile(scenePath)) {
            m_currentScenePath = scenePath;
            m_sceneModified = false;
            deselectAll();
            LIMBO_LOG_EDITOR_INFO("Scene loaded: {}", scenePath.string());
        } else {
            LIMBO_LOG_EDITOR_ERROR("Failed to load scene: {}", serializer.getError());
        }
    } else {
        LIMBO_LOG_EDITOR_WARN("No scene file found at: {}", scenePath.string());
    }
}

void EditorApp::saveScene() {
    if (m_currentScenePath.empty()) {
        saveSceneAs();
        return;
    }

    SceneSerializer serializer(getWorld());
    if (serializer.saveToFile(m_currentScenePath)) {
        m_sceneModified = false;
        LIMBO_LOG_EDITOR_INFO("Scene saved: {}", m_currentScenePath.string());
    } else {
        LIMBO_LOG_EDITOR_ERROR("Failed to save scene: {}", serializer.getError());
    }
}

void EditorApp::saveSceneAs() {
    // TODO: Save file dialog
    // For now, save to default location
    std::filesystem::path const scenePath =
        m_assetManager.getAssetRoot() / "scenes" / "default.json";
    std::filesystem::create_directories(scenePath.parent_path());

    SceneSerializer serializer(getWorld());
    if (serializer.saveToFile(scenePath)) {
        m_currentScenePath = scenePath;
        m_sceneModified = false;
        LIMBO_LOG_EDITOR_INFO("Scene saved: {}", scenePath.string());
    } else {
        LIMBO_LOG_EDITOR_ERROR("Failed to save scene: {}", serializer.getError());
    }
}

void EditorApp::onPlay() {
    if (m_editorState == EditorState::Edit) {
        // Save scene state for restoration on stop
        SceneSerializer serializer(getWorld());
        m_savedSceneState = serializer.serialize();
        m_wasModifiedBeforePlay = m_sceneModified;

        // Clear undo history (play mode changes shouldn't be undone in edit mode)
        m_commandHistory.clear();

        // Deselect entity (selection may become invalid during play)
        deselectAll();

        // Attach physics system to create bodies from components
        m_physicsSystem->onAttach(getWorld());

        // Attach script system (this binds world to script engine)
        m_scriptSystem->onAttach(getWorld());

        // Bind physics to script engine AFTER onAttach (for Physics.raycast, Entity:getRigidbody,
        // etc.) Must be after onAttach because onAttach calls bindWorld which recreates Entity
        // usertype
        m_scriptEngine.bindPhysics(&m_physics);

        // Wire collision events to script callbacks
        m_physicsSystem->setCollisionCallback(
            [this](const CollisionEvent2D& event, CollisionEventType type) {
                m_scriptSystem->dispatchCollisionEvent(getWorld(), event, type);
            });

        m_editorState = EditorState::Play;
        LIMBO_LOG_EDITOR_INFO("Play mode started");
    }
}

void EditorApp::onPause() {
    if (m_editorState == EditorState::Play) {
        m_editorState = EditorState::Pause;
        LIMBO_LOG_EDITOR_INFO("Play mode paused");
    } else if (m_editorState == EditorState::Pause) {
        m_editorState = EditorState::Play;
        LIMBO_LOG_EDITOR_INFO("Play mode resumed");
    }
}

void EditorApp::onStop() {
    if (m_editorState != EditorState::Edit) {
        // Detach script system first (calls onDestroy callbacks)
        m_scriptSystem->onDetach(getWorld());

        // Detach physics system to destroy bodies
        m_physicsSystem->onDetach(getWorld());

        // Restore scene state from before play
        if (!m_savedSceneState.empty()) {
            SceneSerializer serializer(getWorld());
            if (serializer.deserialize(m_savedSceneState)) {
                LIMBO_LOG_EDITOR_INFO("Scene state restored");
            } else {
                LIMBO_LOG_EDITOR_ERROR("Failed to restore scene state: {}", serializer.getError());
            }
            m_savedSceneState.clear();
        }

        // Restore modification flag
        m_sceneModified = m_wasModifiedBeforePlay;

        // Deselect entity (entity IDs may have changed)
        deselectAll();

        m_editorState = EditorState::Edit;
        LIMBO_LOG_EDITOR_INFO("Play mode stopped");
    }
}

void EditorApp::selectEntity(Entity entity) {
    m_selectedEntity = entity;
    m_hierarchyPanel.setSelectedEntity(entity);
    m_inspectorPanel.setSelectedEntity(entity);
}

void EditorApp::deselectAll() {
    m_selectedEntity = Entity();
    m_hierarchyPanel.setSelectedEntity(Entity());
    m_inspectorPanel.setSelectedEntity(Entity());
}

void EditorApp::onShutdown() {
    m_prefabOverridesPanel.shutdown();
    m_consolePanel.shutdown();
    m_assetPipelinePanel.shutdown();
    m_assetBrowserPanel.shutdown();
    m_viewportPanel.shutdown();
    m_inspectorPanel.shutdown();
    m_hierarchyPanel.shutdown();

    m_physics.shutdown();
    m_imguiLayer.shutdown();
    Renderer2D::shutdown();

    if (m_renderContext) {
        m_renderContext->shutdown();
        m_renderContext.reset();
    }

    LIMBO_LOG_EDITOR_INFO("Limbo Editor shutdown");
}

}  // namespace limbo::editor
