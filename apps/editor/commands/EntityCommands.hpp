#pragma once

#include "Command.hpp"

#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Entity.hpp>
#include <limbo/ecs/Components.hpp>
#include <limbo/ecs/Hierarchy.hpp>
#include <limbo/scene/SceneSerializer.hpp>

#include <functional>
#include <optional>

namespace limbo::editor {

// Forward declaration
class EditorApp;

/**
 * CreateEntityCommand - Create a new entity
 */
class CreateEntityCommand : public Command {
public:
    using EntityCreatedCallback = std::function<void(Entity)>;

    CreateEntityCommand(World& world, const String& name, EntityCreatedCallback callback = nullptr)
        : m_world(world), m_name(name), m_callback(std::move(callback)) {}

    bool execute() override {
        m_entity = m_world.createEntity(m_name);
        m_entity.addComponent<TransformComponent>();
        m_entityId = m_entity.id();
        if (m_callback) {
            m_callback(m_entity);
        }
        return true;
    }

    bool undo() override {
        if (m_world.isValid(m_entityId)) {
            Hierarchy::destroyWithChildren(m_world, m_entityId);
        }
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Create Entity '" + m_name + "'"; }

    [[nodiscard]] Entity getCreatedEntity() const { return m_entity; }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    String m_name;
    Entity m_entity;
    World::EntityId m_entityId = World::kNullEntity;
    EntityCreatedCallback m_callback;
};

/**
 * DeleteEntityCommand - Delete an entity (with serialized state for undo)
 */
class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(World& world, World::EntityId entityId)
        : m_world(world), m_entityId(entityId) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId)) {
            return false;
        }

        // Store entity data for undo
        m_storedData = serializeEntity(m_entityId);

        // Store parent for hierarchy restoration
        m_parentId = Hierarchy::getParent(m_world, m_entityId);

        // Delete the entity
        Hierarchy::destroyWithChildren(m_world, m_entityId);
        return true;
    }

    bool undo() override {
        // Recreate entity from stored data
        m_entityId = deserializeEntity(m_storedData);

        // Restore parent
        if (m_parentId != World::kNullEntity && m_world.isValid(m_parentId)) {
            Hierarchy::setParent(m_world, m_entityId, m_parentId);
        }

        return m_entityId != World::kNullEntity;
    }

    [[nodiscard]] String getDescription() const override { return "Delete Entity"; }

    COMMAND_TYPE_ID()

private:
    struct EntityData {
        String name;
        std::optional<TransformComponent> transform;
        std::optional<SpriteRendererComponent> sprite;
        std::optional<CameraComponent> camera;
        bool hasStatic = false;
        bool hasActive = false;
    };

    EntityData serializeEntity(World::EntityId id) {
        EntityData data;

        if (m_world.hasComponent<NameComponent>(id)) {
            data.name = m_world.getComponent<NameComponent>(id).name;
        }
        if (m_world.hasComponent<TransformComponent>(id)) {
            data.transform = m_world.getComponent<TransformComponent>(id);
        }
        if (m_world.hasComponent<SpriteRendererComponent>(id)) {
            data.sprite = m_world.getComponent<SpriteRendererComponent>(id);
        }
        if (m_world.hasComponent<CameraComponent>(id)) {
            data.camera = m_world.getComponent<CameraComponent>(id);
        }
        data.hasStatic = m_world.hasComponent<StaticComponent>(id);
        data.hasActive = m_world.hasComponent<ActiveComponent>(id);

        return data;
    }

    World::EntityId deserializeEntity(const EntityData& data) {
        Entity entity = m_world.createEntity(data.name);

        if (data.transform) {
            entity.addComponent<TransformComponent>(*data.transform);
        }
        if (data.sprite) {
            entity.addComponent<SpriteRendererComponent>(*data.sprite);
        }
        if (data.camera) {
            entity.addComponent<CameraComponent>(*data.camera);
        }
        if (data.hasStatic) {
            entity.addComponent<StaticComponent>();
        }
        if (data.hasActive) {
            entity.addComponent<ActiveComponent>();
        }

        return entity.id();
    }

private:
    World& m_world;
    World::EntityId m_entityId;
    World::EntityId m_parentId = World::kNullEntity;
    EntityData m_storedData;
};

/**
 * ReparentEntityCommand - Change entity's parent
 */
class ReparentEntityCommand : public Command {
public:
    ReparentEntityCommand(World& world, World::EntityId entity, World::EntityId newParent)
        : m_world(world), m_entityId(entity), m_newParentId(newParent) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId)) {
            return false;
        }

        // Store old parent for undo
        m_oldParentId = Hierarchy::getParent(m_world, m_entityId);

        // Set new parent
        if (m_newParentId == World::kNullEntity) {
            Hierarchy::detachFromParent(m_world, m_entityId);
        } else {
            Hierarchy::setParent(m_world, m_entityId, m_newParentId);
        }

        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId)) {
            return false;
        }

        if (m_oldParentId == World::kNullEntity) {
            Hierarchy::detachFromParent(m_world, m_entityId);
        } else if (m_world.isValid(m_oldParentId)) {
            Hierarchy::setParent(m_world, m_entityId, m_oldParentId);
        }

        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Reparent Entity"; }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    World::EntityId m_newParentId;
    World::EntityId m_oldParentId = World::kNullEntity;
};

/**
 * DuplicateEntityCommand - Duplicate an entity
 */
class DuplicateEntityCommand : public Command {
public:
    using EntityCreatedCallback = std::function<void(Entity)>;

    DuplicateEntityCommand(World& world, World::EntityId source,
                           EntityCreatedCallback callback = nullptr)
        : m_world(world), m_sourceId(source), m_callback(std::move(callback)) {}

    bool execute() override {
        if (!m_world.isValid(m_sourceId)) {
            return false;
        }

        // Get source entity data
        String name = "Entity";
        if (m_world.hasComponent<NameComponent>(m_sourceId)) {
            name = m_world.getComponent<NameComponent>(m_sourceId).name + " (Copy)";
        }

        // Create new entity
        Entity newEntity = m_world.createEntity(name);
        m_createdId = newEntity.id();

        // Copy components
        if (m_world.hasComponent<TransformComponent>(m_sourceId)) {
            const auto& src = m_world.getComponent<TransformComponent>(m_sourceId);
            newEntity.addComponent<TransformComponent>(src.position, src.rotation, src.scale);
        }

        if (m_world.hasComponent<SpriteRendererComponent>(m_sourceId)) {
            const auto& src = m_world.getComponent<SpriteRendererComponent>(m_sourceId);
            auto& dest = newEntity.addComponent<SpriteRendererComponent>(src.color);
            dest.textureId = src.textureId;
            dest.sortingOrder = src.sortingOrder;
            dest.uvMin = src.uvMin;
            dest.uvMax = src.uvMax;
        }

        if (m_world.hasComponent<CameraComponent>(m_sourceId)) {
            const auto& src = m_world.getComponent<CameraComponent>(m_sourceId);
            auto& dest = newEntity.addComponent<CameraComponent>();
            dest.projectionType = src.projectionType;
            dest.fov = src.fov;
            dest.orthoSize = src.orthoSize;
            dest.nearClip = src.nearClip;
            dest.farClip = src.farClip;
            dest.primary = false;  // Don't duplicate primary flag
        }

        if (m_world.hasComponent<StaticComponent>(m_sourceId)) {
            newEntity.addComponent<StaticComponent>();
        }

        if (m_world.hasComponent<ActiveComponent>(m_sourceId)) {
            newEntity.addComponent<ActiveComponent>();
        }

        // Copy parent
        World::EntityId parent = Hierarchy::getParent(m_world, m_sourceId);
        if (parent != World::kNullEntity) {
            Hierarchy::setParent(m_world, m_createdId, parent);
        }

        if (m_callback) {
            m_callback(newEntity);
        }

        return true;
    }

    bool undo() override {
        if (m_world.isValid(m_createdId)) {
            Hierarchy::destroyWithChildren(m_world, m_createdId);
        }
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Duplicate Entity"; }

    [[nodiscard]] World::EntityId getCreatedEntityId() const { return m_createdId; }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_sourceId;
    World::EntityId m_createdId = World::kNullEntity;
    EntityCreatedCallback m_callback;
};

/**
 * RenameEntityCommand - Rename an entity
 */
class RenameEntityCommand : public Command {
public:
    RenameEntityCommand(World& world, World::EntityId entity, const String& newName)
        : m_world(world), m_entityId(entity), m_newName(newName) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId) || !m_world.hasComponent<NameComponent>(m_entityId)) {
            return false;
        }

        auto& nameComp = m_world.getComponent<NameComponent>(m_entityId);
        m_oldName = nameComp.name;
        nameComp.name = m_newName;
        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId) || !m_world.hasComponent<NameComponent>(m_entityId)) {
            return false;
        }

        m_world.getComponent<NameComponent>(m_entityId).name = m_oldName;
        return true;
    }

    [[nodiscard]] String getDescription() const override {
        return "Rename Entity to '" + m_newName + "'";
    }

    [[nodiscard]] bool canMergeWith(const Command& other) const override {
        const auto* otherRename = dynamic_cast<const RenameEntityCommand*>(&other);
        return otherRename != nullptr && otherRename->m_entityId == m_entityId;
    }

    void mergeWith(const Command& other) override {
        const auto* otherRename = dynamic_cast<const RenameEntityCommand*>(&other);
        if (otherRename != nullptr) {
            m_newName = otherRename->m_newName;
        }
    }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    String m_newName;
    String m_oldName;
};

}  // namespace limbo::editor
