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
#include "panels/PrefabOverridesPanel.hpp"
#include "panels/ScriptDebugPanel.hpp"
#include "PrefabStage.hpp"

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
    void renderDockspace();
    void setupDockingLayout(ImGuiID dockspaceId);

    // File operations
    void newScene();
    void openScene();
    void saveScene();
    void saveSceneAs();
    void performSaveAs(const std::filesystem::path& path);
    void updateWindowTitle();

    // Play controls
    void onPlay();
    void onPause();
    void onStop();

public:
    // Selection (public for panels)
    void selectEntity(Entity entity);
    void deselectAll();
    [[nodiscard]] Entity getSelectedEntity() const { return m_selectedEntity; }

    // Scene/Prefab modification tracking
    void markSceneModified();

    // Load scene from file path (used by asset browser)
    void loadSceneFromPath(const std::filesystem::path& scenePath);

    // Physics debug visualization
    [[nodiscard]] PhysicsDebug2D& getPhysicsDebug() { return m_physicsDebug; }
    [[nodiscard]] bool isPhysicsDebugEnabled() const { return m_showPhysicsDebug; }
    void setPhysicsDebugEnabled(bool enabled) { m_showPhysicsDebug = enabled; }
    [[nodiscard]] Physics2D& getPhysics() { return m_physics; }
    [[nodiscard]] EditorState getEditorState() const { return m_editorState; }

    // Command system access
    CommandHistory& getCommandHistory() { return m_commandHistory; }

    // Prefab stage access
    PrefabStage& getPrefabStage() { return m_prefabStage; }
    [[nodiscard]] bool isEditingPrefab() const { return m_prefabStage.isOpen(); }

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

    // Asset manager access
    [[nodiscard]] AssetManager& getAssetManager() { return m_assetManager; }
    [[nodiscard]] const AssetManager& getAssetManager() const { return m_assetManager; }

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
    PrefabOverridesPanel m_prefabOverridesPanel;
    ScriptDebugPanel m_scriptDebugPanel;

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

    // Prefab editing
    PrefabStage m_prefabStage;

    // Selection
    Entity m_selectedEntity;

    // Timing
    f32 m_deltaTime = 0.0f;

    // UI state
    bool m_showDemoWindow = false;
    bool m_showSceneSelectPopup = false;
    bool m_showProfiler = false;
    bool m_showPrefabCloseDialog = false;

    // Save As dialog state
    bool m_showSaveAsPopup = false;
    bool m_showOverwriteConfirm = false;
    char m_saveAsFilename[256]{};
    std::filesystem::path m_saveAsTargetPath;

    // Layout state
    bool m_layoutInitialized = false;
    ImGuiID m_dockspaceId = 0;
};

}  // namespace limbo::editor
