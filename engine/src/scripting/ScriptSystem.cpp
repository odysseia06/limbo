#include "limbo/scripting/ScriptSystem.hpp"
#include "limbo/scripting/ScriptComponent.hpp"
#include "limbo/ecs/Entity.hpp"

#include <spdlog/spdlog.h>
#include <regex>

namespace limbo {

namespace {

/**
 * Parse a Sol2 error message to extract file and line number
 * Sol2 errors typically look like:
 *   [string "path/to/script.lua"]:15: error message
 *   or: path/to/script.lua:15: error message
 */
struct ParsedError {
    String message;
    String file;
    i32 line = 0;
};

ParsedError parseScriptError(const String& errorStr) {
    ParsedError result;
    result.message = errorStr;

    // Pattern 1: [string "path"]:line: message
    std::regex const pattern1(R"(\[string \"([^\"]+)\"\]:(\d+):\s*(.*))");
    std::smatch match;

    if (std::regex_search(errorStr, match, pattern1)) {
        result.file = match[1].str();
        result.line = std::stoi(match[2].str());
        result.message = match[3].str();
        return result;
    }

    // Pattern 2: path:line: message (direct file path)
    std::regex const pattern2(R"(([^:]+):(\d+):\s*(.*))");

    if (std::regex_search(errorStr, match, pattern2)) {
        result.file = match[1].str();
        result.line = std::stoi(match[2].str());
        result.message = match[3].str();
        return result;
    }

    // No pattern matched, return raw message
    return result;
}

void setScriptError(ScriptComponent& script, const String& errorStr) {
    auto parsed = parseScriptError(errorStr);
    script.lastError = parsed.message;
    script.lastErrorLine = parsed.line;
    script.enabled = false;

    // Log with file:line info if available
    if (parsed.line > 0) {
        spdlog::error("Script error at {}:{}: {}", parsed.file, parsed.line, parsed.message);
    } else {
        spdlog::error("Script error: {}", errorStr);
    }
}

}  // namespace

ScriptSystem::ScriptSystem(ScriptEngine& engine) : m_engine(engine) {}

void ScriptSystem::onAttach(World& world) {
    // Bind the world to the script engine for entity access
    m_engine.bindWorld(&world);

    // Set up hot reload callback for logging
    m_hotReloadManager.setReloadCallback([](const std::filesystem::path& path, bool success) {
        if (success) {
            spdlog::info("Script reloaded successfully: {}", path.string());
        } else {
            spdlog::error("Script reload failed: {}", path.string());
        }
    });

    spdlog::debug("ScriptSystem initialized with hot reload support");
}

void ScriptSystem::update(World& world, f32 deltaTime) {
    m_totalTime += deltaTime;

    // Poll for script file changes and process hot reloads
    m_hotReloadManager.poll(world);

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
                    try {
                        auto result = script.onStart();
                        if (!result.valid()) {
                            sol::error const err = result;
                            setScriptError(script, err.what());
                        }
                    } catch (const sol::error& e) {
                        setScriptError(script, e.what());
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
    // Stop watching all scripts
    m_hotReloadManager.unwatchAll();

    // Call onDestroy for all scripts
    world.each<ScriptComponent>([](World::EntityId, ScriptComponent& script) {
        if (script.initialized && script.onDestroy.valid()) {
            auto result = script.onDestroy();
            if (!result.valid()) {
                sol::error const err = result;
                spdlog::error("Script onDestroy error: {}", err.what());
            }
        }
    });

    spdlog::debug("ScriptSystem shutdown");
}

void ScriptSystem::initializeScript(World& world, World::EntityId entityId) {
    auto& script = world.getComponent<ScriptComponent>(entityId);

    // Clear previous error state
    script.clearError();

    if (script.scriptPath.empty()) {
        script.initialized = true;
        return;
    }

    if (!std::filesystem::exists(script.scriptPath)) {
        script.lastError = "Script file not found";
        script.enabled = false;
        spdlog::error("Script file not found: {}", script.scriptPath.string());
        return;
    }

    try {
        auto& lua = m_engine.getLuaState();

        // Create a sandboxed environment for this script
        script.environment = sol::environment(lua, sol::create, lua.globals());

        // Add 'self' reference to the entity
        Entity const entity(entityId, &world);
        script.environment["self"] = entity;

        // Load and execute the script in the environment
        auto result = lua.safe_script_file(script.scriptPath.string(), script.environment,
                                           sol::script_pass_on_error);

        if (!result.valid()) {
            sol::error const err = result;
            setScriptError(script, err.what());
            return;
        }

        // Cache function references - lifecycle
        script.onStart = script.environment["onStart"];
        script.onUpdate = script.environment["onUpdate"];
        script.onDestroy = script.environment["onDestroy"];

        // Cache function references - collision callbacks
        script.onCollisionBegin = script.environment["onCollisionBegin"];
        script.onCollisionEnd = script.environment["onCollisionEnd"];
        script.onTriggerEnter = script.environment["onTriggerEnter"];
        script.onTriggerExit = script.environment["onTriggerExit"];

        script.initialized = true;
        spdlog::debug("Loaded script: {}", script.scriptPath.string());

        // Register with hot reload manager
        m_hotReloadManager.watchScript(script.scriptPath, entityId);

    } catch (const sol::error& e) {
        setScriptError(script, e.what());
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
            sol::error const err = result;
            setScriptError(script, err.what());
        }
    } catch (const sol::error& e) {
        setScriptError(script, e.what());
    }
}

void ScriptSystem::dispatchCollisionEvent(World& world, const CollisionEvent2D& event,
                                          CollisionEventType type) {
    // Check if the entity has a script component
    if (!world.hasComponent<ScriptComponent>(event.self)) {
        return;
    }

    auto& script = world.getComponent<ScriptComponent>(event.self);

    // Only dispatch if script is initialized and enabled
    if (!script.initialized || !script.enabled) {
        return;
    }

    // Create Entity wrapper for the other entity
    Entity otherEntity(event.other, &world);

    // Determine which callback to invoke
    sol::protected_function* callback = nullptr;
    bool includeNormal = false;

    if (event.isTrigger) {
        // Trigger events
        callback =
            (type == CollisionEventType::Begin) ? &script.onTriggerEnter : &script.onTriggerExit;
    } else {
        // Collision events
        callback =
            (type == CollisionEventType::Begin) ? &script.onCollisionBegin : &script.onCollisionEnd;
        includeNormal = (type == CollisionEventType::Begin);  // Only Begin has meaningful normal
    }

    if (callback == nullptr || !callback->valid()) {
        return;
    }

    try {
        sol::protected_function_result result;

        if (includeNormal) {
            // onCollisionBegin(other, normal)
            glm::vec2 normal = event.normal;
            result = (*callback)(otherEntity, normal);
        } else {
            // onCollisionEnd(other), onTriggerEnter(other), onTriggerExit(other)
            result = (*callback)(otherEntity);
        }

        if (!result.valid()) {
            sol::error const err = result;
            setScriptError(script, err.what());
        }
    } catch (const sol::error& e) {
        setScriptError(script, e.what());
    }
}

}  // namespace limbo
