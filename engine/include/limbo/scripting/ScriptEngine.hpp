#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"

#include <sol/sol.hpp>
#include <filesystem>
#include <functional>

namespace limbo {

// Forward declarations
class World;

/**
 * ScriptEngine - Manages Lua scripting environment
 *
 * Provides:
 * - Lua state management
 * - Script loading and execution
 * - Engine type bindings (Vec3, Entity, etc.)
 * - Script hot-reloading support
 */
class LIMBO_API ScriptEngine {
public:
    ScriptEngine() = default;
    ~ScriptEngine();

    // Non-copyable
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    /**
     * Initialize the scripting engine
     */
    bool init();

    /**
     * Shutdown the scripting engine
     */
    void shutdown();

    /**
     * Check if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * Get the Lua state
     */
    sol::state& getLuaState() { return m_lua; }
    const sol::state& getLuaState() const { return m_lua; }

    /**
     * Load and execute a script file
     * @return true on success
     */
    bool loadScript(const std::filesystem::path& path);

    /**
     * Execute a Lua string
     * @return true on success
     */
    bool executeString(const String& code);

    /**
     * Call a global Lua function
     */
    template <typename... Args>
    sol::protected_function_result callFunction(const String& name, Args&&... args) {
        sol::protected_function func = m_lua[name];
        if (!func.valid()) {
            return sol::protected_function_result();
        }
        return func(std::forward<Args>(args)...);
    }

    /**
     * Check if a global function exists
     */
    bool hasFunction(const String& name) const;

    /**
     * Register engine bindings (called during init)
     */
    void registerBindings();

    /**
     * Bind the World for entity access
     */
    void bindWorld(World* world);

    /**
     * Get last error message
     */
    const String& getLastError() const { return m_lastError; }

private:
    void bindMathTypes();
    void bindInputTypes();
    void bindEntityTypes();
    void bindComponentTypes();
    void bindUtilityFunctions();

    sol::state m_lua;
    bool m_initialized = false;
    String m_lastError;
    World* m_boundWorld = nullptr;
};

}  // namespace limbo
