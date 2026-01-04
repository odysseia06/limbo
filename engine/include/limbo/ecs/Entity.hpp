#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/World.hpp"

namespace limbo {

/**
 * Entity - A lightweight wrapper around an entity ID
 *
 * Provides a convenient object-oriented interface for working with entities.
 * Entity objects are cheap to copy and compare.
 */
class LIMBO_API Entity {
public:
    Entity() = default;
    Entity(World::EntityId id, World* world) : m_id(id), m_world(world) {}

    // ========================================================================
    // Validity and Comparison
    // ========================================================================

    /**
     * Check if this entity is valid (has a valid ID and world)
     */
    [[nodiscard]] bool isValid() const { return m_world != nullptr && m_world->isValid(m_id); }

    /**
     * Implicit bool conversion for validity checking
     */
    [[nodiscard]] explicit operator bool() const { return isValid(); }

    /**
     * Get the raw entity ID
     */
    [[nodiscard]] World::EntityId id() const { return m_id; }

    /**
     * Get the world this entity belongs to
     */
    [[nodiscard]] World* world() const { return m_world; }

    // Comparison operators
    [[nodiscard]] bool operator==(const Entity& other) const {
        return m_id == other.m_id && m_world == other.m_world;
    }

    [[nodiscard]] bool operator!=(const Entity& other) const { return !(*this == other); }

    // ========================================================================
    // Component Operations
    // ========================================================================

    /**
     * Add a component to this entity
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Reference to the added component (for non-empty types)
     */
    template <typename T, typename... Args>
    decltype(auto) addComponent(Args&&... args) {
        return m_world->addComponent<T>(m_id, std::forward<Args>(args)...);
    }

    /**
     * Get or add a component to this entity
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments (used if component doesn't exist)
     * @return Reference to the component
     */
    template <typename T, typename... Args>
    T& getOrAddComponent(Args&&... args) {
        return m_world->getOrAddComponent<T>(m_id, std::forward<Args>(args)...);
    }

    /**
     * Remove a component from this entity
     * @tparam T Component type
     */
    template <typename T>
    void removeComponent() {
        m_world->removeComponent<T>(m_id);
    }

    /**
     * Check if this entity has a component
     * @tparam T Component type
     * @return True if the entity has the component
     */
    template <typename T>
    [[nodiscard]] bool hasComponent() const {
        return m_world->hasComponent<T>(m_id);
    }

    /**
     * Check if this entity has all specified components
     * @tparam Ts Component types
     * @return True if the entity has all components
     */
    template <typename... Ts>
    [[nodiscard]] bool hasAllComponents() const {
        return m_world->hasAllComponents<Ts...>(m_id);
    }

    /**
     * Check if this entity has any of the specified components
     * @tparam Ts Component types
     * @return True if the entity has at least one component
     */
    template <typename... Ts>
    [[nodiscard]] bool hasAnyComponent() const {
        return m_world->hasAnyComponent<Ts...>(m_id);
    }

    /**
     * Get a component from this entity
     * @tparam T Component type
     * @return Reference to the component
     */
    template <typename T>
    [[nodiscard]] T& getComponent() {
        return m_world->getComponent<T>(m_id);
    }

    /**
     * Get a component from this entity (const version)
     * @tparam T Component type
     * @return Const reference to the component
     */
    template <typename T>
    [[nodiscard]] const T& getComponent() const {
        return m_world->getComponent<T>(m_id);
    }

    /**
     * Try to get a component from this entity
     * @tparam T Component type
     * @return Pointer to the component, or nullptr if not found
     */
    template <typename T>
    [[nodiscard]] T* tryGetComponent() {
        return m_world->tryGetComponent<T>(m_id);
    }

    /**
     * Try to get a component from this entity (const version)
     * @tparam T Component type
     * @return Const pointer to the component, or nullptr if not found
     */
    template <typename T>
    [[nodiscard]] const T* tryGetComponent() const {
        return m_world->tryGetComponent<T>(m_id);
    }

    /**
     * Destroy this entity
     */
    void destroy() {
        if (m_world) {
            m_world->destroyEntity(m_id);
            m_id = World::kNullEntity;
            m_world = nullptr;
        }
    }

private:
    World::EntityId m_id = World::kNullEntity;
    World* m_world = nullptr;
};

}  // namespace limbo
