#pragma once

#include <limbo/Limbo.hpp>

namespace limbo::editor {

class EditorApp;

/**
 * SceneHierarchyPanel - Displays the entity hierarchy tree
 */
class SceneHierarchyPanel {
public:
    explicit SceneHierarchyPanel(EditorApp& editor);

    void init();
    void shutdown();
    void render();

    void setSelectedEntity(Entity entity) { m_selectedEntity = entity; }
    [[nodiscard]] Entity getSelectedEntity() const { return m_selectedEntity; }

    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawEntityNode(Entity entity);
    void drawContextMenu();

private:
    EditorApp& m_editor;
    Entity m_selectedEntity;
    bool m_open = true;
};

} // namespace limbo::editor
