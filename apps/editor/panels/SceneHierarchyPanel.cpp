#include "SceneHierarchyPanel.hpp"
#include "EditorApp.hpp"
#include "../commands/EntityCommands.hpp"

#include <limbo/ecs/Hierarchy.hpp>

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

    // Draw only root entities (those without parents or without HierarchyComponent)
    world.each<NameComponent>([this, &world](World::EntityId id, NameComponent&) {
        // Skip entities that have a parent
        if (world.hasComponent<HierarchyComponent>(id)) {
            const auto& hierarchy = world.getComponent<HierarchyComponent>(id);
            if (hierarchy.hasParent()) {
                return;  // Will be drawn as a child of its parent
            }
        }

        Entity const entity(id, &m_editor.getWorld());
        drawEntityNode(entity);
    });

    // Right-click context menu on empty space
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu",
                                       ImGuiPopupFlags_NoOpenOverItems |
                                           ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Create Empty Entity")) {
            auto cmd = std::make_unique<CreateEntityCommand>(
                m_editor.getWorld(), "New Entity", [this](Entity e) { m_editor.selectEntity(e); });
            m_editor.executeCommand(std::move(cmd));
        }
        if (ImGui::MenuItem("Create Sprite")) {
            auto cmd = std::make_unique<CreateEntityCommand>(
                m_editor.getWorld(), "Sprite", [this](Entity e) {
                    e.addComponent<SpriteRendererComponent>(glm::vec4(1.0f));
                    m_editor.selectEntity(e);
                });
            m_editor.executeCommand(std::move(cmd));
        }
        ImGui::EndPopup();
    }

    // Handle drag-drop to reparent to root (empty space)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE")) {
            auto droppedEntityId = *static_cast<World::EntityId*>(payload->Data);
            auto cmd = std::make_unique<ReparentEntityCommand>(m_editor.getWorld(), droppedEntityId,
                                                               World::kNullEntity);
            m_editor.executeCommand(std::move(cmd));
        }
        ImGui::EndDragDropTarget();
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

    auto& world = m_editor.getWorld();
    auto& nameComp = entity.getComponent<NameComponent>();

    // Check if entity has children
    bool hasChildren = false;
    if (world.hasComponent<HierarchyComponent>(entity.id())) {
        hasChildren = world.getComponent<HierarchyComponent>(entity.id()).hasChildren();
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
                               ImGuiTreeNodeFlags_DefaultOpen;

    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (m_selectedEntity.isValid() && m_selectedEntity.id() == entity.id()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool const opened = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<u32>(entity.id()))), flags, "%s",
        nameComp.name.c_str());

    // Selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        m_selectedEntity = entity;
        m_editor.selectEntity(entity);
    }

    // Drag source - allow dragging this entity
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        World::EntityId id = entity.id();
        ImGui::SetDragDropPayload("ENTITY_NODE", &id, sizeof(World::EntityId));
        ImGui::Text("%s", nameComp.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target - allow dropping entities onto this to reparent
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE")) {
            auto droppedEntityId = *static_cast<World::EntityId*>(payload->Data);

            // Don't allow parenting to self or creating cycles
            if (droppedEntityId != entity.id() &&
                !Hierarchy::isAncestorOf(world, droppedEntityId, entity.id())) {
                auto cmd =
                    std::make_unique<ReparentEntityCommand>(world, droppedEntityId, entity.id());
                m_editor.executeCommand(std::move(cmd));
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child")) {
            auto cmd = std::make_unique<CreateEntityCommand>(
                world, "New Child", [this, entityId = entity.id()](Entity child) {
                    Hierarchy::setParent(m_editor.getWorld(), child.id(), entityId);
                    m_editor.selectEntity(child);
                });
            m_editor.executeCommand(std::move(cmd));
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Duplicate")) {
            duplicateEntity(entity);
        }
        if (ImGui::MenuItem("Delete")) {
            deleteEntity(entity);
        }
        ImGui::Separator();
        if (Hierarchy::getParent(world, entity.id()) != World::kNullEntity) {
            if (ImGui::MenuItem("Unparent")) {
                auto cmd =
                    std::make_unique<ReparentEntityCommand>(world, entity.id(), World::kNullEntity);
                m_editor.executeCommand(std::move(cmd));
            }
        }
        ImGui::EndPopup();
    }

    // Draw children if opened
    if (opened) {
        if (hasChildren) {
            Hierarchy::forEachChild(world, entity.id(), [this, &world](World::EntityId childId) {
                Entity childEntity(childId, &m_editor.getWorld());
                drawEntityNode(childEntity);
                return true;
            });
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::duplicateEntity(Entity entity) {
    if (!entity.isValid()) {
        return;
    }

    auto cmd = std::make_unique<DuplicateEntityCommand>(m_editor.getWorld(), entity.id(),
                                                        [this](Entity newEntity) {
                                                            m_selectedEntity = newEntity;
                                                            m_editor.selectEntity(newEntity);
                                                        });
    m_editor.executeCommand(std::move(cmd));
}

void SceneHierarchyPanel::deleteEntity(Entity entity) {
    if (!entity.isValid()) {
        return;
    }

    bool wasSelected = m_selectedEntity.isValid() && m_selectedEntity.id() == entity.id();

    auto cmd = std::make_unique<DeleteEntityCommand>(m_editor.getWorld(), entity.id());
    m_editor.executeCommand(std::move(cmd));

    if (wasSelected) {
        m_selectedEntity = Entity();
        m_editor.deselectAll();
    }
}

void SceneHierarchyPanel::drawContextMenu() {}

}  // namespace limbo::editor
