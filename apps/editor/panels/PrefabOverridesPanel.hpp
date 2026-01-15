#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/scene/Prefab.hpp>

namespace limbo::editor {

class EditorApp;

/// Panel for viewing and managing prefab instance overrides
/// Shows all property overrides for the selected prefab instance
/// with per-property Apply/Revert functionality
class PrefabOverridesPanel {
public:
    explicit PrefabOverridesPanel(EditorApp& editor);

    void init();
    void shutdown();
    void render();

    void setOpen(bool open) { m_open = open; }
    [[nodiscard]] bool isOpen() const { return m_open; }
    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawOverridesList();
    void drawOverrideRow(Entity entity, PrefabInstanceComponent& prefabInst,
                         const PrefabOverride& override, usize index);
    void drawEmptyState();
    void drawToolbar();

    // Helper to get display name for component.property
    [[nodiscard]] String getPropertyDisplayName(const String& component,
                                                const String& property) const;

    // Find all prefab instances in the selected entity hierarchy
    void collectPrefabInstances(Entity root, std::vector<Entity>& outInstances);

    EditorApp& m_editor;
    bool m_open = true;

    // Filter
    char m_filterBuffer[128] = {0};
};

}  // namespace limbo::editor
