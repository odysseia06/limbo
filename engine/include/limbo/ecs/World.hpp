#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <entt/entt.hpp>

#include <functional>

namespace limbo {

// Forward declaration
class Entity;

/**
 * World - Manages all entities and their components using EnTT
 *
 * The World is the central container for all game entities. It wraps
 * entt::registry and provides a cleaner API for common operations.
 */
class LIMBO_API World {
public:
    using EntityId = entt::entity;
    static constexpr EntityId NullEntity = entt::null;

    World() = default;
    ~World() = default;

    // Non-copyable, moveable
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) noexcept = default;
    World& operator=(World&&) noexcept = default;

    // ========================================================================
    // Entity Management
    // ========================================================================

    /**
     * Create a new entity
     * @return The entity wrapper
     */
    Entity createEntity();

    /**
     * Create a new entity with a name
     * @param name The entity name
     * @return The entity wrapper
     */
    Entity createEntity(StringView name);

    /**
     * Destroy an entity and all its components
     * @param entity The entity to destroy
     */
    void destroyEntity(EntityId entity);

    /**
     * Check if an entity is valid (exists in this world)
     * @param entity The entity to check
     * @return True if the entity exists
     */
    [[nodiscard]] bool isValid(EntityId entity) const;

    /**
     * Get the number of entities in the world
     * @return Entity count
     */
    [[nodiscard]] usize entityCount() const;

    /**
     * Clear all entities and components
     */
    void clear();

    // ========================================================================
    // Component Operations (direct registry access)
    // ========================================================================

    /**
     * Add a component to an entity
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param entity The entity
     * @param args Constructor arguments
     * @return Reference to the added component (for non-empty types)
     */
    template<typename T, typename... Args>
    decltype(auto) addComponent(EntityId entity, Args&&... args) {
        if constexpr (std::is_empty_v<T>) {
            m_registry.emplace<T>(entity, std::forward<Args>(args)...);
        } else {
            return m_registry.emplace<T>(entity, std::forward<Args>(args)...);
        }
    }

    /**
     * Get or add a component to an entity
     * @tparam T Component type
     * @tparam Args Constructor argument types
     * @param entity The entity
     * @param args Constructor arguments (used if component doesn't exist)
     * @return Reference to the component
     */
    template<typename T, typename... Args>
    T& getOrAddComponent(EntityId entity, Args&&... args) {
        return m_registry.get_or_emplace<T>(entity, std::forward<Args>(args)...);
    }

    /**
     * Remove a component from an entity
     * @tparam T Component type
     * @param entity The entity
     */
    template<typename T>
    void removeComponent(EntityId entity) {
        m_registry.remove<T>(entity);
    }

    /**
     * Check if an entity has a component
     * @tparam T Component type
     * @param entity The entity
     * @return True if the entity has the component
     */
    template<typename T>
    [[nodiscard]] bool hasComponent(EntityId entity) const {
        return m_registry.all_of<T>(entity);
    }

    /**
     * Check if an entity has all specified components
     * @tparam Ts Component types
     * @param entity The entity
     * @return True if the entity has all components
     */
    template<typename... Ts>
    [[nodiscard]] bool hasAllComponents(EntityId entity) const {
        return m_registry.all_of<Ts...>(entity);
    }

    /**
     * Check if an entity has any of the specified components
     * @tparam Ts Component types
     * @param entity The entity
     * @return True if the entity has at least one component
     */
    template<typename... Ts>
    [[nodiscard]] bool hasAnyComponent(EntityId entity) const {
        return m_registry.any_of<Ts...>(entity);
    }

    /**
     * Get a component from an entity
     * @tparam T Component type
     * @param entity The entity
     * @return Reference to the component
     */
    template<typename T>
    [[nodiscard]] T& getComponent(EntityId entity) {
        return m_registry.get<T>(entity);
    }

    /**
     * Get a component from an entity (const version)
     * @tparam T Component type
     * @param entity The entity
     * @return Const reference to the component
     */
    template<typename T>
    [[nodiscard]] const T& getComponent(EntityId entity) const {
        return m_registry.get<T>(entity);
    }

    /**
     * Try to get a component from an entity
     * @tparam T Component type
     * @param entity The entity
     * @return Pointer to the component, or nullptr if not found
     */
    template<typename T>
    [[nodiscard]] T* tryGetComponent(EntityId entity) {
        return m_registry.try_get<T>(entity);
    }

    /**
     * Try to get a component from an entity (const version)
     * @tparam T Component type
     * @param entity The entity
     * @return Const pointer to the component, or nullptr if not found
     */
    template<typename T>
    [[nodiscard]] const T* tryGetComponent(EntityId entity) const {
        return m_registry.try_get<T>(entity);
    }

    // ========================================================================
    // Views and Iteration
    // ========================================================================

    /**
     * Get a view of entities with specified components
     * @tparam Ts Component types to include
     * @return View for iteration
     */
    template<typename... Ts>
    [[nodiscard]] auto view() {
        return m_registry.view<Ts...>();
    }

    /**
     * Get a view of entities with specified components (const version)
     * @tparam Ts Component types to include
     * @return View for iteration
     */
    template<typename... Ts>
    [[nodiscard]] auto view() const {
        return m_registry.view<Ts...>();
    }

    /**
     * Iterate over entities with specified components
     * @tparam Ts Component types
     * @param func Callback function(EntityId, Ts&...)
     */
    template<typename... Ts, typename Func>
    void each(Func&& func) {
        m_registry.view<Ts...>().each(std::forward<Func>(func));
    }

    // ========================================================================
    // Registry Access (for advanced usage)
    // ========================================================================

    /**
     * Get direct access to the underlying registry
     * Use sparingly - prefer the World API when possible
     */
    [[nodiscard]] entt::registry& registry() { return m_registry; }
    [[nodiscard]] const entt::registry& registry() const { return m_registry; }

private:
    entt::registry m_registry;
};

} // namespace limbo
