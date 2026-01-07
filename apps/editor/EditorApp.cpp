#include "EditorApp.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace limbo::editor {

EditorApp::EditorApp()
    : m_hierarchyPanel(*this), m_inspectorPanel(*this), m_viewportPanel(*this),
      m_assetBrowserPanel(*this), m_assetPipelinePanel(*this) {}

void EditorApp::onInit() {
    spdlog::info("Limbo Editor initialized");

    // Create render context
    m_renderContext = RenderContext::create();
    if (!m_renderContext->init(getWindow())) {
        spdlog::critical("Failed to initialize render context");
        requestExit();
        return;
    }

    // Initialize Renderer2D
    Renderer2D::init();

    // Initialize ImGui
    if (!m_imguiLayer.init(getWindow().getNativeHandle())) {
        spdlog::error("Failed to initialize ImGui");
    }

    // Initialize camera
    float const aspect =
        static_cast<float>(getWindow().getWidth()) / static_cast<float>(getWindow().getHeight());
    m_editorCamera = OrthographicCamera(-aspect * m_cameraZoom, aspect * m_cameraZoom,
                                        -m_cameraZoom, m_cameraZoom);

    // Initialize physics (for play mode)
    m_physics.init({0.0f, -9.81f});

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

    // Start with a new scene
    newScene();

    spdlog::info("Editor ready");
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
    }

    // Update based on editor state
    if (m_editorState == EditorState::Play) {
        // Run physics and systems
        getSystems().update(getWorld(), deltaTime);
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
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Delete", false, m_selectedEntity.isValid())) {
                if (m_selectedEntity.isValid()) {
                    getWorld().destroyEntity(m_selectedEntity.id());
                    deselectAll();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity")) {
            if (ImGui::MenuItem("Create Empty")) {
                auto entity = getWorld().createEntity("New Entity");
                entity.addComponent<TransformComponent>();
                selectEntity(entity);
            }
            if (ImGui::MenuItem("Create Sprite")) {
                auto entity = getWorld().createEntity("Sprite");
                entity.addComponent<TransformComponent>();
                entity.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));
                selectEntity(entity);
            }
            if (ImGui::MenuItem("Create Camera")) {
                auto entity = getWorld().createEntity("Camera");
                entity.addComponent<TransformComponent>();
                // TODO: Add CameraComponent when implemented
                selectEntity(entity);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_hierarchyPanel.isOpen());
            ImGui::MenuItem("Inspector", nullptr, &m_inspectorPanel.isOpen());
            ImGui::MenuItem("Viewport", nullptr, &m_viewportPanel.isOpen());
            ImGui::MenuItem("Asset Browser", nullptr, &m_assetBrowserPanel.isOpen());
            ImGui::MenuItem("Asset Pipeline", nullptr, &m_assetPipelinePanel.isOpen());
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

        ImGui::EndMenuBar();
    }
}

void EditorApp::renderToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    ImGuiWindowFlags const flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##Toolbar", nullptr, flags)) {
        // Play/Pause/Stop buttons
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

        ImGui::SameLine();

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

        ImGui::SameLine();

        // Stop button
        ImGui::BeginDisabled(m_editorState == EditorState::Edit);
        if (ImGui::Button("Stop")) {
            onStop();
        }
        ImGui::EndDisabled();

        // Scene name on the right
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        String sceneName =
            m_currentScenePath.empty() ? "Untitled" : m_currentScenePath.filename().string();
        if (m_sceneModified) {
            sceneName += "*";
        }
        ImGui::Text("Scene: %s", sceneName.c_str());
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
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

    // Dockspace
    ImGuiID const dockspaceId = ImGui::GetID("EditorDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();

    // Toolbar
    renderToolbar();

    // Render panels
    m_hierarchyPanel.render();
    m_inspectorPanel.render();
    m_viewportPanel.render();
    m_assetBrowserPanel.render();
    m_assetPipelinePanel.render();

    // Status bar
    renderStatusBar();
}

void EditorApp::renderStatusBar() {
    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Status Bar", nullptr, flags);
    ImGui::Text("FPS: %.0f | Entities: %zu | Draw Calls: %u | Quads: %u",
                static_cast<double>(1.0f / m_deltaTime), getWorld().entityCount(),
                Renderer2D::getStats().drawCalls, Renderer2D::getStats().quadCount);
    ImGui::End();
}

void EditorApp::newScene() {
    getWorld().clear();
    m_currentScenePath.clear();
    m_sceneModified = false;
    deselectAll();
    spdlog::info("New scene created");
}

void EditorApp::openScene() {
    // TODO: Open file dialog
    // For now, try to load a default scene
    std::filesystem::path const scenePath =
        m_assetManager.getAssetRoot() / "scenes" / "default.json";

    if (std::filesystem::exists(scenePath)) {
        SceneSerializer serializer(getWorld());
        if (serializer.loadFromFile(scenePath)) {
            m_currentScenePath = scenePath;
            m_sceneModified = false;
            deselectAll();
            spdlog::info("Scene loaded: {}", scenePath.string());
        } else {
            spdlog::error("Failed to load scene: {}", serializer.getError());
        }
    } else {
        spdlog::warn("No scene file found at: {}", scenePath.string());
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
        spdlog::info("Scene saved: {}", m_currentScenePath.string());
    } else {
        spdlog::error("Failed to save scene: {}", serializer.getError());
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
        spdlog::info("Scene saved: {}", scenePath.string());
    } else {
        spdlog::error("Failed to save scene: {}", serializer.getError());
    }
}

void EditorApp::onPlay() {
    if (m_editorState == EditorState::Edit) {
        // TODO: Save scene state for restoration on stop
        m_editorState = EditorState::Play;
        spdlog::info("Play mode started");
    }
}

void EditorApp::onPause() {
    if (m_editorState == EditorState::Play) {
        m_editorState = EditorState::Pause;
        spdlog::info("Play mode paused");
    } else if (m_editorState == EditorState::Pause) {
        m_editorState = EditorState::Play;
        spdlog::info("Play mode resumed");
    }
}

void EditorApp::onStop() {
    if (m_editorState != EditorState::Edit) {
        // TODO: Restore scene state
        m_editorState = EditorState::Edit;
        spdlog::info("Play mode stopped");
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

    spdlog::info("Limbo Editor shutdown");
}

}  // namespace limbo::editor
