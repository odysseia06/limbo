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

    /// Cached function references
    sol::protected_function onStart;
    sol::protected_function onUpdate;
    sol::protected_function onDestroy;

    /// Whether the script has been initialized
    bool initialized = false;

    /// Whether onStart has been called
    bool started = false;

    /// Whether the script is enabled
    bool enabled = true;

    ScriptComponent() = default;
    explicit ScriptComponent(const std::filesystem::path& path)
        : scriptPath(path) {}
};

} // namespace limbo
