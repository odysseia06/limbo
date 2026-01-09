#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"

#include <sol/sol.hpp>
#include <filesystem>

namespace limbo {

/**
 * ScriptComponent - ECS component for Lua scripting
 *
 * Attach to an entity to give it scripted behavior.
 * Scripts can implement onStart(), onUpdate(dt), and onDestroy() callbacks.
 */
struct LIMBO_API ScriptComponent {
    /// Path to the script file
    std::filesystem::path scriptPath;

    /// Script environment (sandboxed per-entity)
    sol::environment environment;

    /// Cached function references - lifecycle
    sol::protected_function onStart;
    sol::protected_function onUpdate;
    sol::protected_function onDestroy;

    /// Cached function references - collision callbacks
    sol::protected_function onCollisionBegin;  // (other: Entity, normal: Vec2)
    sol::protected_function onCollisionEnd;    // (other: Entity)
    sol::protected_function onTriggerEnter;    // (other: Entity)
    sol::protected_function onTriggerExit;     // (other: Entity)

    /// Whether the script has been initialized
    bool initialized = false;

    /// Whether onStart has been called
    bool started = false;

    /// Whether the script is enabled
    bool enabled = true;

    /// Last error message (empty if no error)
    String lastError;

    /// Line number of last error (0 if unknown)
    i32 lastErrorLine = 0;

    ScriptComponent() = default;
    explicit ScriptComponent(const std::filesystem::path& path) : scriptPath(path) {}

    /// Clear error state
    void clearError() {
        lastError.clear();
        lastErrorLine = 0;
    }

    /// Check if there's an error
    [[nodiscard]] bool hasError() const { return !lastError.empty(); }
};

}  // namespace limbo
