#include "SceneHierarchyPanel.hpp"
#include "EditorApp.hpp"

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

    // Handle drag-drop to reparent to root (empty space)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE")) {
            auto droppedEntityId = *static_cast<World::EntityId*>(payload->Data);
            Hierarchy::detachFromParent(m_editor.getWorld(), droppedEntityId);
            m_editor.markSceneModified();
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

<<<<<<< HEAD
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
=======
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_SpanAvailWidth |
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0
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
                Hierarchy::setParent(world, droppedEntityId, entity.id());
                m_editor.markSceneModified();
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child")) {
            auto child = world.createEntity("New Child");
            child.addComponent<TransformComponent>();
            Hierarchy::setParent(world, child.id(), entity.id());
            m_editor.markSceneModified();
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
                Hierarchy::detachFromParent(world, entity.id());
                m_editor.markSceneModified();
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

    auto& world = m_editor.getWorld();

    // Create new entity with same name + " (Copy)"
    auto& nameComp = entity.getComponent<NameComponent>();
    auto newEntity = world.createEntity(nameComp.name + " (Copy)");

    // Copy TransformComponent
    if (entity.hasComponent<TransformComponent>()) {
        const auto& transform = entity.getComponent<TransformComponent>();
        newEntity.addComponent<TransformComponent>(transform.position, transform.rotation,
                                                   transform.scale);
    }

    // Copy SpriteRendererComponent
    if (entity.hasComponent<SpriteRendererComponent>()) {
        const auto& sprite = entity.getComponent<SpriteRendererComponent>();
        auto& newSprite = newEntity.addComponent<SpriteRendererComponent>(sprite.color);
        newSprite.textureId = sprite.textureId;
        newSprite.sortingOrder = sprite.sortingOrder;
        newSprite.uvMin = sprite.uvMin;
        newSprite.uvMax = sprite.uvMax;
    }

    // Copy CameraComponent
    if (entity.hasComponent<CameraComponent>()) {
        const auto& camera = entity.getComponent<CameraComponent>();
        auto& newCamera = newEntity.addComponent<CameraComponent>();
        newCamera.projectionType = camera.projectionType;
        newCamera.fov = camera.fov;
        newCamera.orthoSize = camera.orthoSize;
        newCamera.nearClip = camera.nearClip;
        newCamera.farClip = camera.farClip;
        newCamera.primary = false;  // Don't duplicate primary flag
    }

    // Copy tag components
    if (entity.hasComponent<StaticComponent>()) {
        newEntity.addComponent<StaticComponent>();
    }
    if (entity.hasComponent<ActiveComponent>()) {
        newEntity.addComponent<ActiveComponent>();
    }

    // If original has a parent, parent the copy to the same parent
    World::EntityId parent = Hierarchy::getParent(world, entity.id());
    if (parent != World::kNullEntity) {
        Hierarchy::setParent(world, newEntity.id(), parent);
    }

    m_selectedEntity = newEntity;
    m_editor.selectEntity(newEntity);
    m_editor.markSceneModified();
}

void SceneHierarchyPanel::deleteEntity(Entity entity) {
    if (!entity.isValid()) {
        return;
    }

    auto& world = m_editor.getWorld();

    // Destroy with children
    Hierarchy::destroyWithChildren(world, entity.id());

    if (m_selectedEntity.id() == entity.id()) {
        m_selectedEntity = Entity();
        m_editor.deselectAll();
    }

    m_editor.markSceneModified();
}

void SceneHierarchyPanel::drawContextMenu() {}

}  // namespace limbo::editor
