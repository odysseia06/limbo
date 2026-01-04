#include "SceneHierarchyPanel.hpp"
#include "EditorApp.hpp"

#include <imgui.h>

namespace limbo::editor {

SceneHierarchyPanel::SceneHierarchyPanel(EditorApp& editor) : m_editor(editor) {}

void SceneHierarchyPanel::init() {}

void SceneHierarchyPanel::shutdown() {}

void SceneHierarchyPanel::render() {
    if (!m_open) {
        return;
}

    ImGui::Begin("Hierarchy", &m_open);

    auto& world = m_editor.getWorld();

    // Draw all entities
    world.each<NameComponent>([this](World::EntityId id, NameComponent&) {
        Entity const entity(id, &m_editor.getWorld());
        drawEntityNode(entity);
    });

    // Right-click context menu on empty space
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu",
                                       ImGuiPopupFlags_NoOpenOverItems |
                                           ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Create Empty Entity")) {
            auto entity = m_editor.getWorld().createEntity("New Entity");
            entity.addComponent<TransformComponent>();
        }
        if (ImGui::MenuItem("Create Sprite")) {
            auto entity = m_editor.getWorld().createEntity("Sprite");
            entity.addComponent<TransformComponent>();
            entity.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));
        }
        ImGui::EndPopup();
    }

    // Deselect on click empty space
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
        m_selectedEntity = Entity();
        m_editor.deselectAll();
    }

    ImGui::End();
}

void SceneHierarchyPanel::drawEntityNode(Entity entity) {
    if (!entity.isValid()) {
        return;
}

    auto& nameComp = entity.getComponent<NameComponent>();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= ImGuiTreeNodeFlags_Leaf;  // No children for now

    if (m_selectedEntity.isValid() && m_selectedEntity.id() == entity.id()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool const opened = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<u32>(entity.id()))), flags, "%s",
        nameComp.name.c_str());

    // Selection
    if (ImGui::IsItemClicked()) {
        m_selectedEntity = entity;
        m_editor.selectEntity(entity);
    }

    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
            m_editor.getWorld().destroyEntity(entity.id());
            if (m_selectedEntity.id() == entity.id()) {
                m_selectedEntity = Entity();
                m_editor.deselectAll();
            }
        }
        if (ImGui::MenuItem("Duplicate")) {
            // TODO: Implement entity duplication
        }
        ImGui::EndPopup();
    }

    if (opened) {
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::drawContextMenu() {}

}  // namespace limbo::editor
