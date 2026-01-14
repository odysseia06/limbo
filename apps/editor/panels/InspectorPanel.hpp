#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/scripting/ScriptComponent.hpp>
#include <limbo/scene/Prefab.hpp>

namespace limbo::editor {

class EditorApp;

/**
 * InspectorPanel - Shows and edits properties of the selected entity
 */
class InspectorPanel {
public:
    explicit InspectorPanel(EditorApp& editor);

    void init();
    void shutdown();
    void render();

    void setSelectedEntity(Entity entity) { m_selectedEntity = entity; }

    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawComponents();
    void drawAddComponentMenu();
    void drawPrefabSection();

    // Component drawers
    void drawNameComponent(NameComponent& component);
    void drawTransformComponent(TransformComponent& component);
    void drawSpriteRendererComponent(SpriteRendererComponent& component);
    void drawQuadRendererComponent(QuadRendererComponent& component);
    void drawCircleRendererComponent(CircleRendererComponent& component);
    void drawTextRendererComponent(TextRendererComponent& component);
    void drawRigidbody2DComponent(Rigidbody2DComponent& component);
    void drawBoxCollider2DComponent(BoxCollider2DComponent& component);
    void drawCircleCollider2DComponent(CircleCollider2DComponent& component);
    void drawScriptComponent(ScriptComponent& component);

    // Helper to list available script files
    void drawScriptFilePicker(ScriptComponent& component);

    template <typename T>
    void drawComponentHeader(const char* name, bool removable = true);

private:
    EditorApp& m_editor;
    Entity m_selectedEntity;
    bool m_open = true;
};

}  // namespace limbo::editor
