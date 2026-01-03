#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/scripting/ScriptEngine.hpp"

namespace limbo {

/**
 * ScriptSystem - ECS system that executes Lua scripts
 *
 * Manages the lifecycle of script components:
 * - Loads and initializes scripts on first update
 * - Calls onStart() once when script is ready
 * - Calls onUpdate(dt) each frame
 * - Calls onDestroy() when entity is destroyed
 */
class LIMBO_API ScriptSystem : public System {
public:
    explicit ScriptSystem(ScriptEngine& engine);
    ~ScriptSystem() override = default;

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;

private:
    void initializeScript(World& world, World::EntityId entity);
    void updateScript(World& world, World::EntityId entity, f32 deltaTime);

    ScriptEngine& m_engine;
    f32 m_totalTime = 0.0f;
};

} // namespace limbo
