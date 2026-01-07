#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"
#include "limbo/assets/FileWatcher.hpp"

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace limbo {

// Forward declarations
class Asset;
class AssetRegistry;

/**
 * Reload event data
 */
struct ReloadEvent {
    AssetId assetId;
    std::filesystem::path path;
    bool success = false;
    String error;
};

/**
 * Reload callback types
 */
using ReloadCallback = std::function<void(const ReloadEvent&)>;
using BeforeReloadCallback = std::function<bool(AssetId)>;  // Return false to cancel reload

/**
 * HotReloadManager - Manages hot-reloading of assets with dependency tracking
 *
 * Features:
 * - Dependency graph tracking (when A changes, reload B that depends on A)
 * - Batched reloads to avoid redundant operations
 * - Configurable reload callbacks
 * - Integration with FileWatcher for file change detection
 *
 * Usage:
 *   HotReloadManager manager;
 *   manager.setEnabled(true);
 *   manager.watchAsset(assetId, assetPath);
 *   manager.addDependency(materialId, textureId);  // material depends on texture
 *
 *   // In update loop:
 *   manager.poll();
 */
class LIMBO_API HotReloadManager {
public:
    HotReloadManager() = default;
    ~HotReloadManager() = default;

    // Non-copyable
    HotReloadManager(const HotReloadManager&) = delete;
    HotReloadManager& operator=(const HotReloadManager&) = delete;

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
     * Set whether to batch reloads (process all pending changes at once)
     * Default is true, which reduces redundant reloads
     */
    void setBatchReloads(bool batch) { m_batchReloads = batch; }

    // ========================================================================
    // Asset Watching
    // ========================================================================

    /**
     * Start watching an asset for changes
     * @param id Asset ID
     * @param path File path to watch
     */
    void watchAsset(AssetId id, const std::filesystem::path& path);

    /**
     * Watch multiple paths for a single asset (e.g., atlas with texture)
     */
    void watchAsset(AssetId id, const std::vector<std::filesystem::path>& paths);

    /**
     * Stop watching an asset
     */
    void unwatchAsset(AssetId id);

    /**
     * Stop watching all assets
     */
    void unwatchAll();

    /**
     * Check if an asset is being watched
     */
    [[nodiscard]] bool isWatching(AssetId id) const;

    // ========================================================================
    // Dependency Tracking
    // ========================================================================

    /**
     * Register that assetId depends on dependencyId
     * When dependencyId changes, assetId will be scheduled for reload
     * @param assetId The asset that depends on another
     * @param dependencyId The asset being depended upon
     */
    void addDependency(AssetId assetId, AssetId dependencyId);

    /**
     * Remove a dependency
     */
    void removeDependency(AssetId assetId, AssetId dependencyId);

    /**
     * Clear all dependencies for an asset
     */
    void clearDependencies(AssetId assetId);

    /**
     * Get all assets that depend on a given asset (direct dependents)
     */
    [[nodiscard]] std::vector<AssetId> getDependents(AssetId id) const;

    /**
     * Get all assets that will be affected by changing a given asset
     * (transitive closure of dependents)
     */
    [[nodiscard]] std::vector<AssetId> getAffectedAssets(AssetId id) const;

    /**
     * Get all dependencies of an asset
     */
    [[nodiscard]] std::vector<AssetId> getDependencies(AssetId id) const;

    // ========================================================================
    // Reload Control
    // ========================================================================

    /**
     * Set the reload handler function
     * This is called to actually perform the reload of an asset
     */
    void setReloadHandler(std::function<bool(AssetId)> handler) {
        m_reloadHandler = std::move(handler);
    }

    /**
     * Set callback for before reload (can cancel)
     */
    void setBeforeReloadCallback(BeforeReloadCallback callback) {
        m_beforeReloadCallback = std::move(callback);
    }

    /**
     * Set callback for after reload
     */
    void setAfterReloadCallback(ReloadCallback callback) {
        m_afterReloadCallback = std::move(callback);
    }

    /**
     * Manually trigger a reload of an asset and its dependents
     */
    void triggerReload(AssetId id);

    /**
     * Poll for file changes and process pending reloads
     * Call this regularly (e.g., once per frame)
     */
    void poll();

    /**
     * Process all pending reloads immediately
     */
    void processPendingReloads();

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
    struct WatchedAsset {
        AssetId id;
        std::vector<std::filesystem::path> paths;
    };

    FileWatcher m_fileWatcher;
    bool m_enabled = false;
    bool m_batchReloads = true;

    // Asset -> paths mapping
    std::unordered_map<AssetId, WatchedAsset> m_watchedAssets;

    // Path -> asset ID mapping (for reverse lookup when file changes)
    std::unordered_map<String, AssetId> m_pathToAsset;

    // Dependency graph: asset -> set of assets it depends on
    std::unordered_map<AssetId, std::unordered_set<AssetId>> m_dependencies;

    // Reverse dependency graph: asset -> set of assets that depend on it
    std::unordered_map<AssetId, std::unordered_set<AssetId>> m_dependents;

    // Pending reloads (set to avoid duplicates)
    std::unordered_set<AssetId> m_pendingReloads;

    // Callbacks
    std::function<bool(AssetId)> m_reloadHandler;
    BeforeReloadCallback m_beforeReloadCallback;
    ReloadCallback m_afterReloadCallback;

    // Statistics
    u32 m_totalReloads = 0;
    u32 m_failedReloads = 0;

    /**
     * Called when a file changes
     */
    void onFileChanged(const std::filesystem::path& path);

    /**
     * Collect all affected assets using BFS/DFS on dependency graph
     */
    void collectAffectedAssets(AssetId id, std::unordered_set<AssetId>& affected) const;

    /**
     * Perform topological sort for reload order
     * Assets should be reloaded in dependency order (dependencies first)
     */
    [[nodiscard]] std::vector<AssetId>
    topologicalSort(const std::unordered_set<AssetId>& assets) const;
};

}  // namespace limbo
