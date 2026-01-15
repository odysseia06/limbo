#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/scripting/ScriptSystem.hpp>

namespace limbo::editor {

class EditorApp;

/**
 * ScriptDebugPanel - Debug and monitor Lua scripts
 *
 * Features:
 * - List all scripts in the scene with status
 * - Show error details with file:line
 * - Hot reload statistics
 * - Manual reload button
 * - Filter by status (error/running/all)
 */
class ScriptDebugPanel {
public:
    explicit ScriptDebugPanel(EditorApp& editor);
    ~ScriptDebugPanel() = default;

    void render(World& world, ScriptSystem* scriptSystem);

    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawToolbar(ScriptSystem* scriptSystem);
    void drawScriptList(World& world, ScriptSystem* scriptSystem);
    void drawScriptDetails(World& world);

private:
    EditorApp& m_editor;
    bool m_open = false;

    // Filter settings
    bool m_showRunning = true;
    bool m_showErrors = true;
    bool m_showPending = true;
    bool m_showDisabled = true;

    // Selection
    World::EntityId m_selectedScript = entt::null;
};

}  // namespace limbo::editor
