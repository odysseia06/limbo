#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/physics/2d/PhysicsSystem2D.hpp>
#include <limbo/physics/2d/PhysicsDebug2D.hpp>
#include <limbo/scripting/ScriptEngine.hpp>
#include <limbo/scripting/ScriptSystem.hpp>
#include "commands/CommandHistory.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "panels/AssetBrowserPanel.hpp"
#include "panels/AssetPipelinePanel.hpp"
#include "panels/ConsolePanel.hpp"

#include <filesystem>

namespace limbo::editor {

/**
 * Editor play state
 */
enum class EditorState {
    Edit,  // Editing mode - scene is not running
    Play,  // Play mode - scene is simulating
    Pause  // Paused - simulation paused
};

/**
 * EditorApp - The main Limbo Editor application
 *
 * Provides a full-featured level editor with:
 * - Scene hierarchy view
 * - Entity inspector/properties
 * - Viewport with camera controls and gizmos
 * - Asset browser
 * - Play/Pause/Stop controls
 */
class EditorApp : public Application {
public:
    EditorApp();
    ~EditorApp() override = default;

protected:
    void onInit() override;
    void onUpdate(f32 deltaTime) override;
    void onRender(f32 interpolationAlpha) override;
    void onShutdown() override;

private:
    // UI rendering
    void renderMenuBar();
    void renderToolbar();
    void renderDockspace();
    void renderStatusBar();

    // File operations
    void newScene();
    void openScene();
    void loadSceneFromPath(const std::filesystem::path& scenePath);
    void saveScene();
    void saveSceneAs();

    // Play controls
    void onPlay();
    void onPause();
    void onStop();

public:
    // Selection (public for panels)
    void selectEntity(Entity entity);
    void deselectAll();
    [[nodiscard]] Entity getSelectedEntity() const { return m_selectedEntity; }

    // Scene modification tracking
    void markSceneModified() { m_sceneModified = true; }

    // Physics debug visualization
    [[nodiscard]] PhysicsDebug2D& getPhysicsDebug() { return m_physicsDebug; }
    [[nodiscard]] bool isPhysicsDebugEnabled() const { return m_showPhysicsDebug; }
    void setPhysicsDebugEnabled(bool enabled) { m_showPhysicsDebug = enabled; }
    [[nodiscard]] Physics2D& getPhysics() { return m_physics; }
    [[nodiscard]] EditorState getEditorState() const { return m_editorState; }

    // Command system access
    CommandHistory& getCommandHistory() { return m_commandHistory; }

    /**
     * Execute a command through the undo/redo system
     * @param command The command to execute
     * @return True if the command executed successfully
     */
    bool executeCommand(Unique<Command> command);

    /**
     * Undo the last command
     */
    void undo();

    /**
     * Redo the last undone command
     */
    void redo();

private:
    // Rendering
    Unique<RenderContext> m_renderContext;
    OrthographicCamera m_editorCamera;
    f32 m_cameraZoom = 1.0f;

    // ImGui
    ImGuiLayer m_imguiLayer;

    // Panels
    SceneHierarchyPanel m_hierarchyPanel;
    InspectorPanel m_inspectorPanel;
    ViewportPanel m_viewportPanel;
    AssetBrowserPanel m_assetBrowserPanel;
    AssetPipelinePanel m_assetPipelinePanel;
    ConsolePanel m_consolePanel;

    // Assets
    AssetManager m_assetManager;

    // Physics (for play mode)
    Physics2D m_physics;
    Unique<PhysicsSystem2D> m_physicsSystem;
    PhysicsDebug2D m_physicsDebug;
    bool m_showPhysicsDebug = true;

    // Scripting (for play mode)
    ScriptEngine m_scriptEngine;
    Unique<ScriptSystem> m_scriptSystem;

    // Editor state
    EditorState m_editorState = EditorState::Edit;
    std::filesystem::path m_currentScenePath;
    bool m_sceneModified = false;

    // Play mode state preservation
    String m_savedSceneState;  // Serialized scene before play
    bool m_wasModifiedBeforePlay = false;

    // Command history for undo/redo
    CommandHistory m_commandHistory;

    // Selection
    Entity m_selectedEntity;

    // Timing
    f32 m_deltaTime = 0.0f;

    // UI state
    bool m_showDemoWindow = false;
    bool m_showSceneSelectPopup = false;

    // Layout persistence
    std::string m_layoutIniPath;
};

}  // namespace limbo::editor
