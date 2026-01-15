#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/scene/Prefab.hpp>

#include <filesystem>
#include <functional>

namespace limbo::editor {

class EditorApp;

/**
 * PrefabStage - Manages isolated prefab editing
 *
 * When editing a prefab, the PrefabStage:
 * 1. Saves the current scene state
 * 2. Creates a temporary world with just the prefab contents
 * 3. Allows editing the prefab in isolation
 * 4. On save, writes changes back to the .prefab file
 * 5. On close, restores the original scene
 *
 * This follows the Unity-style "Prefab Mode" workflow.
 */
class PrefabStage {
public:
    explicit PrefabStage(EditorApp& editor);
    ~PrefabStage() = default;

    /**
     * Open a prefab for editing
     * @param prefabPath Path to the .prefab file
     * @return True if the prefab was opened successfully
     */
    bool open(const std::filesystem::path& prefabPath);

    /**
     * Open a prefab for editing from a prefab instance
     * @param prefabId The UUID of the prefab to edit
     * @return True if the prefab was opened successfully
     */
    bool openFromInstance(const UUID& prefabId);

    /**
     * Save changes to the prefab
     * @return True if the save was successful
     */
    bool save();

    /**
     * Close the prefab stage and return to scene editing
     * @param saveChanges If true, save changes before closing
     */
    void close(bool saveChanges = false);

    /**
     * Check if a prefab is currently being edited
     */
    [[nodiscard]] bool isOpen() const { return m_isOpen; }

    /**
     * Check if the prefab has unsaved changes
     */
    [[nodiscard]] bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }

    /**
     * Get the path to the currently edited prefab
     */
    [[nodiscard]] const std::filesystem::path& getPrefabPath() const { return m_prefabPath; }

    /**
     * Get the name of the currently edited prefab
     */
    [[nodiscard]] const String& getPrefabName() const { return m_prefabName; }

    /**
     * Mark the prefab as having unsaved changes
     */
    void markModified() { m_hasUnsavedChanges = true; }

    /**
     * Get the prefab world for editing
     * Note: Only valid when isOpen() returns true
     */
    [[nodiscard]] World& getPrefabWorld() { return m_prefabWorld; }

    /**
     * Update instances of this prefab in the main scene after save
     */
    void updateSceneInstances();

private:
    EditorApp& m_editor;

    // State
    bool m_isOpen = false;
    bool m_hasUnsavedChanges = false;

    // Prefab being edited
    std::filesystem::path m_prefabPath;
    String m_prefabName;
    Prefab m_prefab;

    // Isolated world for prefab editing
    World m_prefabWorld;

    // Saved scene state (serialized JSON) to restore when closing
    String m_savedSceneState;
    std::filesystem::path m_savedScenePath;
    Entity m_savedSelection;
};

}  // namespace limbo::editor
