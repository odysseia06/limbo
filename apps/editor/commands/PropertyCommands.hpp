#pragma once

#include "Command.hpp"

#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Components.hpp>

#include <glm/glm.hpp>

namespace limbo::editor {

/**
 * SetTransformCommand - Modify transform component
 */
class SetTransformCommand : public Command {
public:
    enum class Property { Position, Rotation, Scale, All };

    SetTransformCommand(World& world, World::EntityId entity, const TransformComponent& newValue,
                        Property property = Property::All)
        : m_world(world), m_entityId(entity), m_newValue(newValue), m_property(property) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId) ||
            !m_world.hasComponent<TransformComponent>(m_entityId)) {
            return false;
        }

        auto& transform = m_world.getComponent<TransformComponent>(m_entityId);
        m_oldValue = transform;

        switch (m_property) {
            case Property::Position:
                transform.position = m_newValue.position;
                break;
            case Property::Rotation:
                transform.rotation = m_newValue.rotation;
                break;
            case Property::Scale:
                transform.scale = m_newValue.scale;
                break;
            case Property::All:
                transform = m_newValue;
                break;
        }

        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId) ||
            !m_world.hasComponent<TransformComponent>(m_entityId)) {
            return false;
        }

        auto& transform = m_world.getComponent<TransformComponent>(m_entityId);

        switch (m_property) {
            case Property::Position:
                transform.position = m_oldValue.position;
                break;
            case Property::Rotation:
                transform.rotation = m_oldValue.rotation;
                break;
            case Property::Scale:
                transform.scale = m_oldValue.scale;
                break;
            case Property::All:
                transform = m_oldValue;
                break;
        }

        return true;
    }

    [[nodiscard]] String getDescription() const override {
        switch (m_property) {
            case Property::Position:
                return "Set Position";
            case Property::Rotation:
                return "Set Rotation";
            case Property::Scale:
                return "Set Scale";
            default:
                return "Set Transform";
        }
    }

    [[nodiscard]] bool canMergeWith(const Command& other) const override {
        const auto* otherTransform = dynamic_cast<const SetTransformCommand*>(&other);
        return otherTransform != nullptr && otherTransform->m_entityId == m_entityId &&
               otherTransform->m_property == m_property;
    }

    void mergeWith(const Command& other) override {
        const auto* otherTransform = dynamic_cast<const SetTransformCommand*>(&other);
        if (otherTransform != nullptr) {
            m_newValue = otherTransform->m_newValue;
        }
    }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    TransformComponent m_newValue;
    TransformComponent m_oldValue;
    Property m_property;
};

/**
 * SetSpriteColorCommand - Modify sprite color
 */
class SetSpriteColorCommand : public Command {
public:
    SetSpriteColorCommand(World& world, World::EntityId entity, const glm::vec4& newColor)
        : m_world(world), m_entityId(entity), m_newColor(newColor) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId) ||
            !m_world.hasComponent<SpriteRendererComponent>(m_entityId)) {
            return false;
        }

        auto& sprite = m_world.getComponent<SpriteRendererComponent>(m_entityId);
        m_oldColor = sprite.color;
        sprite.color = m_newColor;
        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId) ||
            !m_world.hasComponent<SpriteRendererComponent>(m_entityId)) {
            return false;
        }

        m_world.getComponent<SpriteRendererComponent>(m_entityId).color = m_oldColor;
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Set Sprite Color"; }

    [[nodiscard]] bool canMergeWith(const Command& other) const override {
        const auto* otherColor = dynamic_cast<const SetSpriteColorCommand*>(&other);
        return otherColor != nullptr && otherColor->m_entityId == m_entityId;
    }

    void mergeWith(const Command& other) override {
        const auto* otherColor = dynamic_cast<const SetSpriteColorCommand*>(&other);
        if (otherColor != nullptr) {
            m_newColor = otherColor->m_newColor;
        }
    }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    glm::vec4 m_newColor;
    glm::vec4 m_oldColor;
};

/**
 * AddComponentCommand - Add a component to an entity
 */
template <typename T>
class AddComponentCommand : public Command {
public:
    AddComponentCommand(World& world, World::EntityId entity, const String& componentName)
        : m_world(world), m_entityId(entity), m_componentName(componentName) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId) || m_world.hasComponent<T>(m_entityId)) {
            return false;
        }

        m_world.addComponent<T>(m_entityId);
        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId) || !m_world.hasComponent<T>(m_entityId)) {
            return false;
        }

        m_world.removeComponent<T>(m_entityId);
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Add " + m_componentName; }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    String m_componentName;
};

/**
 * RemoveComponentCommand - Remove a component from an entity
 */
template <typename T>
class RemoveComponentCommand : public Command {
public:
    RemoveComponentCommand(World& world, World::EntityId entity, const String& componentName)
        : m_world(world), m_entityId(entity), m_componentName(componentName) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId) || !m_world.hasComponent<T>(m_entityId)) {
            return false;
        }

        // Store component data for undo
        m_storedComponent = m_world.getComponent<T>(m_entityId);
        m_world.removeComponent<T>(m_entityId);
        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId) || m_world.hasComponent<T>(m_entityId)) {
            return false;
        }

        m_world.addComponent<T>(m_entityId, m_storedComponent);
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Remove " + m_componentName; }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    String m_componentName;
    T m_storedComponent;
};

/**
 * SetCameraPropertyCommand - Modify camera component properties
 */
class SetCameraPropertyCommand : public Command {
public:
    SetCameraPropertyCommand(World& world, World::EntityId entity,
                             const CameraComponent& newValue)
        : m_world(world), m_entityId(entity), m_newValue(newValue) {}

    bool execute() override {
        if (!m_world.isValid(m_entityId) || !m_world.hasComponent<CameraComponent>(m_entityId)) {
            return false;
        }

        auto& camera = m_world.getComponent<CameraComponent>(m_entityId);
        m_oldValue = camera;
        camera = m_newValue;
        return true;
    }

    bool undo() override {
        if (!m_world.isValid(m_entityId) || !m_world.hasComponent<CameraComponent>(m_entityId)) {
            return false;
        }

        m_world.getComponent<CameraComponent>(m_entityId) = m_oldValue;
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Set Camera Properties"; }

    [[nodiscard]] bool canMergeWith(const Command& other) const override {
        const auto* otherCamera = dynamic_cast<const SetCameraPropertyCommand*>(&other);
        return otherCamera != nullptr && otherCamera->m_entityId == m_entityId;
    }

    void mergeWith(const Command& other) override {
        const auto* otherCamera = dynamic_cast<const SetCameraPropertyCommand*>(&other);
        if (otherCamera != nullptr) {
            m_newValue = otherCamera->m_newValue;
        }
    }

    COMMAND_TYPE_ID()

private:
    World& m_world;
    World::EntityId m_entityId;
    CameraComponent m_newValue;
    CameraComponent m_oldValue;
};

}  // namespace limbo::editor
