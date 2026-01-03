#include "limbo/scripting/ScriptSystem.hpp"
#include "limbo/scripting/ScriptComponent.hpp"
#include "limbo/ecs/Entity.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

ScriptSystem::ScriptSystem(ScriptEngine& engine) : m_engine(engine) {}

void ScriptSystem::onAttach(World& world) {
    // Bind the world to the script engine for entity access
    m_engine.bindWorld(&world);
    spdlog::debug("ScriptSystem initialized");
}

void ScriptSystem::update(World& world, f32 deltaTime) {
    m_totalTime += deltaTime;

    // Update time globals in Lua
    m_engine.getLuaState()["Time"]["deltaTime"] = deltaTime;
    m_engine.getLuaState()["Time"]["totalTime"] = m_totalTime;

    // Process all entities with ScriptComponent
    world.each<ScriptComponent>(
        [this, &world, deltaTime](World::EntityId entity, ScriptComponent& script) {
            if (!script.enabled) {
                return;
            }

            // Initialize script if not yet done
            if (!script.initialized) {
                initializeScript(world, entity);
            }

            // Call onStart once
            if (script.initialized && !script.started) {
                script.started = true;
                if (script.onStart.valid()) {
                    auto result = script.onStart();
                    if (!result.valid()) {
                        sol::error err = result;
                        spdlog::error("Script onStart error: {}", err.what());
                    }
                }
            }

            // Call onUpdate each frame
            if (script.initialized && script.started) {
                updateScript(world, entity, deltaTime);
            }
        });
}

void ScriptSystem::onDetach(World& world) {
    // Call onDestroy for all scripts
    world.each<ScriptComponent>([](World::EntityId, ScriptComponent& script) {
        if (script.initialized && script.onDestroy.valid()) {
            auto result = script.onDestroy();
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("Script onDestroy error: {}", err.what());
            }
        }
    });

    spdlog::debug("ScriptSystem shutdown");
}

void ScriptSystem::initializeScript(World& world, World::EntityId entityId) {
    auto& script = world.getComponent<ScriptComponent>(entityId);

    if (script.scriptPath.empty()) {
        script.initialized = true;
        return;
    }

    if (!std::filesystem::exists(script.scriptPath)) {
        spdlog::error("Script file not found: {}", script.scriptPath.string());
        script.enabled = false;
        return;
    }

    try {
        auto& lua = m_engine.getLuaState();

        // Create a sandboxed environment for this script
        script.environment = sol::environment(lua, sol::create, lua.globals());

        // Add 'self' reference to the entity
        Entity entity(entityId, &world);
        script.environment["self"] = entity;

        // Load and execute the script in the environment
        auto result = lua.safe_script_file(script.scriptPath.string(), script.environment,
                                           sol::script_pass_on_error);

        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("Script load error in {}: {}", script.scriptPath.string(), err.what());
            script.enabled = false;
            return;
        }

        // Cache function references
        script.onStart = script.environment["onStart"];
        script.onUpdate = script.environment["onUpdate"];
        script.onDestroy = script.environment["onDestroy"];

        script.initialized = true;
        spdlog::debug("Loaded script: {}", script.scriptPath.string());

    } catch (const sol::error& e) {
        spdlog::error("Script exception in {}: {}", script.scriptPath.string(), e.what());
        script.enabled = false;
    }
}

void ScriptSystem::updateScript(World& world, World::EntityId entityId, f32 deltaTime) {
    auto& script = world.getComponent<ScriptComponent>(entityId);

    if (!script.onUpdate.valid()) {
        return;
    }

    try {
        auto result = script.onUpdate(deltaTime);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("Script onUpdate error: {}", err.what());
        }
    } catch (const sol::error& e) {
        spdlog::error("Script exception in onUpdate: {}", e.what());
    }
}

}  // namespace limbo
