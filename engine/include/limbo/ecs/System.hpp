#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <algorithm>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace limbo {

// Forward declaration
class World;

/**
 * System - Base class for all ECS systems
 *
 * Systems contain the logic that operates on entities with specific components.
 * Override the update() method to implement your game logic.
 */
class LIMBO_API System {
public:
    virtual ~System() = default;

    /**
     * Called when the system is added to the world
     * @param world The world this system belongs to
     */
    virtual void onAttach(World& world) { (void)world; }

    /**
     * Called when the system is removed from the world
     * @param world The world this system belonged to
     */
    virtual void onDetach(World& world) { (void)world; }

    /**
     * Called every frame to update the system
     * @param world The world containing entities
     * @param deltaTime Time since last frame in seconds
     */
    virtual void update(World& world, f32 deltaTime) = 0;

    /**
     * Called at a fixed timestep (for physics, etc.)
     * @param world The world containing entities
     * @param fixedDeltaTime Fixed timestep in seconds
     */
    virtual void fixedUpdate(World& world, f32 fixedDeltaTime) {
        (void)world;
        (void)fixedDeltaTime;
    }

    /**
     * Get the system's enabled state
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * Set the system's enabled state
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * Get the system's priority (lower = runs first)
     */
    [[nodiscard]] i32 priority() const { return m_priority; }

    /**
     * Set the system's priority
     */
    void setPriority(i32 priority) { m_priority = priority; }

protected:
    bool m_enabled = true;
    i32 m_priority = 0;
};

/**
 * SystemManager - Manages and runs all systems in a world
 */
class LIMBO_API SystemManager {
public:
    SystemManager() = default;
    ~SystemManager();

    // Non-copyable
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;

    /**
     * Add a system to the manager
     * @tparam T System type (must derive from System)
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the added system
     */
    template <typename T, typename... Args>
    T* addSystem(Args&&... args) {
        static_assert(std::is_base_of_v<System, T>, "T must derive from System");
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        m_systems.push_back(std::move(system));
        m_systemMap[std::type_index(typeid(T))] = ptr;
        m_sorted = false;
        return ptr;
    }

    /**
     * Get a system by type
     * @tparam T System type
     * @return Pointer to the system, or nullptr if not found
     */
    template <typename T>
    [[nodiscard]] T* getSystem() {
        auto it = m_systemMap.find(std::type_index(typeid(T)));
        if (it != m_systemMap.end()) {
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }

    /**
     * Get a system by type (const version)
     * @tparam T System type
     * @return Const pointer to the system, or nullptr if not found
     */
    template <typename T>
    [[nodiscard]] const T* getSystem() const {
        auto it = m_systemMap.find(std::type_index(typeid(T)));
        if (it != m_systemMap.end()) {
            return static_cast<const T*>(it->second);
        }
        return nullptr;
    }

    /**
     * Check if a system exists
     * @tparam T System type
     * @return True if the system exists
     */
    template <typename T>
    [[nodiscard]] bool hasSystem() const {
        return m_systemMap.contains(std::type_index(typeid(T)));
    }

    /**
     * Remove a system by type
     * @tparam T System type
     * @return True if the system was removed
     */
    template <typename T>
    bool removeSystem() {
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_systemMap.find(typeIdx);
        if (it == m_systemMap.end()) {
            return false;
        }

        System const* ptr = it->second;
        m_systemMap.erase(it);

        auto sysIt =
            std::find_if(m_systems.begin(), m_systems.end(),
                         [ptr](const std::unique_ptr<System>& s) { return s.get() == ptr; });

        if (sysIt != m_systems.end()) {
            m_systems.erase(sysIt);
        }

        return true;
    }

    /**
     * Initialize all systems (call onAttach)
     * @param world The world
     */
    void init(World& world);

    /**
     * Shutdown all systems (call onDetach)
     * @param world The world
     */
    void shutdown(World& world);

    /**
     * Update all enabled systems
     * @param world The world
     * @param deltaTime Time since last frame
     */
    void update(World& world, f32 deltaTime);

    /**
     * Fixed update all enabled systems
     * @param world The world
     * @param fixedDeltaTime Fixed timestep
     */
    void fixedUpdate(World& world, f32 fixedDeltaTime);

    /**
     * Get the number of systems
     */
    [[nodiscard]] usize systemCount() const { return m_systems.size(); }

private:
    void sortSystems();

    std::vector<std::unique_ptr<System>> m_systems;
    std::unordered_map<std::type_index, System*> m_systemMap;
    bool m_sorted = true;
};

}  // namespace limbo
