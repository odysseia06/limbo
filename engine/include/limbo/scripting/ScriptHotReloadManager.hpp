#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/FileWatcher.hpp"
#include "limbo/ecs/World.hpp"

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace limbo {

/**
 * ScriptHotReloadManager - Hot-reload Lua scripts during Play mode
 *
 * Watches script files for changes and schedules them for reload.
 * Scripts are reset and re-initialized on the next frame, preserving
 * any state via optional onBeforeReload/onAfterReload callbacks.
 *
 * Usage:
 *   ScriptHotReloadManager manager;
 *   manager.setEnabled(true);
 *   manager.watchScript("assets/scripts/player.lua", entityId);
 *
 *   // In update loop:
 *   manager.poll(world);
 */
class LIMBO_API ScriptHotReloadManager {
public:
    using ReloadCallback = std::function<void(const std::filesystem::path&, bool success)>;

    ScriptHotReloadManager() = default;
    ~ScriptHotReloadManager() = default;

    // Non-copyable
    ScriptHotReloadManager(const ScriptHotReloadManager&) = delete;
    ScriptHotReloadManager& operator=(const ScriptHotReloadManager&) = delete;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Enable or disable hot-reloading
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * Check if hot-reloading is enabled
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * Set the poll interval for file changes
     */
    void setPollInterval(std::chrono::milliseconds interval) {
        m_fileWatcher.setPollInterval(interval);
    }

    /**
     * Set callback for after reload (for logging/UI updates)
     */
    void setReloadCallback(ReloadCallback callback) { m_reloadCallback = std::move(callback); }

    // ========================================================================
    // Script Watching
    // ========================================================================

    /**
     * Start watching a script file for an entity
     * Multiple entities can use the same script file
     * @param path Script file path
     * @param entityId Entity that uses this script
     */
    void watchScript(const std::filesystem::path& path, World::EntityId entityId);

    /**
     * Stop watching a script for an entity
     */
    void unwatchScript(const std::filesystem::path& path, World::EntityId entityId);

    /**
     * Stop watching all scripts for an entity
     */
    void unwatchEntity(World::EntityId entityId);

    /**
     * Stop watching all scripts
     */
    void unwatchAll();

    /**
     * Check if a script is being watched
     */
    [[nodiscard]] bool isWatching(const std::filesystem::path& path) const;

    // ========================================================================
    // Reload Control
    // ========================================================================

    /**
     * Poll for file changes and process pending reloads
     * Call this once per frame during Play mode
     * @param world The ECS world
     */
    void poll(World& world);

    /**
     * Manually trigger a reload of a specific script
     */
    void triggerReload(const std::filesystem::path& path);

    /**
     * Get count of pending reloads
     */
    [[nodiscard]] usize getPendingReloadCount() const { return m_pendingReloads.size(); }

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * Get total number of reloads performed
     */
    [[nodiscard]] u32 getTotalReloads() const { return m_totalReloads; }

    /**
     * Get number of failed reloads
     */
    [[nodiscard]] u32 getFailedReloads() const { return m_failedReloads; }

    /**
     * Reset statistics
     */
    void resetStats() {
        m_totalReloads = 0;
        m_failedReloads = 0;
    }

private:
    FileWatcher m_fileWatcher;
    bool m_enabled = true;

    // Script path -> set of entity IDs using that script
    std::unordered_map<String, std::unordered_set<World::EntityId>> m_scriptToEntities;

    // Pending reloads (script paths)
    std::unordered_set<String> m_pendingReloads;

    // Callback for reload events
    ReloadCallback m_reloadCallback;

    // Statistics
    u32 m_totalReloads = 0;
    u32 m_failedReloads = 0;

    /**
     * Called when a script file changes
     */
    void onScriptChanged(const std::filesystem::path& path);

    /**
     * Perform the actual reload of a script for all entities using it
     */
    void reloadScript(World& world, const std::filesystem::path& path);
};

}  // namespace limbo
