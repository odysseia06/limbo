#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/scripting/ScriptEngine.hpp"
#include "limbo/scripting/ScriptHotReloadManager.hpp"
#include "limbo/physics/2d/ContactListener2D.hpp"

namespace limbo {

/**
 * ScriptSystem - ECS system that executes Lua scripts
 *
 * Manages the lifecycle of script components:
 * - Loads and initializes scripts on first update
 * - Calls onStart() once when script is ready
 * - Calls onUpdate(dt) each frame
 * - Calls onDestroy() when entity is destroyed
 * - Hot-reloads scripts when files change
 */
class LIMBO_API ScriptSystem : public System {
public:
    explicit ScriptSystem(ScriptEngine& engine);
    ~ScriptSystem() override = default;

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;

    /**
     * Dispatch a collision event to a script's callback
     * Called by PhysicsSystem2D after physics step completes
     */
    void dispatchCollisionEvent(World& world, const CollisionEvent2D& event,
                                CollisionEventType type);

    /**
     * Get the hot reload manager for configuration
     */
    [[nodiscard]] ScriptHotReloadManager& getHotReloadManager() { return m_hotReloadManager; }

    /**
     * Enable or disable hot reloading
     */
    void setHotReloadEnabled(bool enabled) { m_hotReloadManager.setEnabled(enabled); }

    /**
     * Check if hot reloading is enabled
     */
    [[nodiscard]] bool isHotReloadEnabled() const { return m_hotReloadManager.isEnabled(); }

private:
    void initializeScript(World& world, World::EntityId entity);
    void updateScript(World& world, World::EntityId entity, f32 deltaTime);

    ScriptEngine& m_engine;
    ScriptHotReloadManager m_hotReloadManager;
    f32 m_totalTime = 0.0f;
};

}  // namespace limbo
