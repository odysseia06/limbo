#include "limbo/assets/HotReloadManager.hpp"

#include "limbo/debug/Log.hpp"

#include <algorithm>
#include <queue>

namespace limbo {

void HotReloadManager::watchAsset(AssetId id, const std::filesystem::path& path) {
    watchAsset(id, std::vector<std::filesystem::path>{path});
}

void HotReloadManager::watchAsset(AssetId id, const std::vector<std::filesystem::path>& paths) {
    if (!id.isValid()) {
        LIMBO_LOG_ASSET_WARN("HotReloadManager: Cannot watch invalid asset ID");
        return;
    }

    // Remove any existing watch for this asset
    unwatchAsset(id);

    WatchedAsset watched;
    watched.id = id;
    watched.paths = paths;

    for (const auto& path : paths) {
        if (!std::filesystem::exists(path)) {
            LIMBO_LOG_ASSET_WARN("HotReloadManager: Path does not exist: {}", path.string());
            continue;
        }

        String const pathKey = path.generic_string();
        m_pathToAsset[pathKey] = id;

        m_fileWatcher.watch(
            path, [this](const std::filesystem::path& changedPath) { onFileChanged(changedPath); });
    }

    m_watchedAssets[id] = std::move(watched);
    LIMBO_LOG_ASSET_DEBUG("HotReloadManager: Watching asset {} ({} files)", id.toString(),
                          paths.size());
}

void HotReloadManager::unwatchAsset(AssetId id) {
    auto it = m_watchedAssets.find(id);
    if (it == m_watchedAssets.end()) {
        return;
    }

    for (const auto& path : it->second.paths) {
        String const pathKey = path.generic_string();
        m_pathToAsset.erase(pathKey);
        m_fileWatcher.unwatch(path);
    }

    m_watchedAssets.erase(it);
    clearDependencies(id);

    // Remove from dependents of other assets
    for (auto& [otherId, deps] : m_dependents) {
        deps.erase(id);
    }

    m_dependents.erase(id);
    m_pendingReloads.erase(id);

    LIMBO_LOG_ASSET_DEBUG("HotReloadManager: Unwatched asset {}", id.toString());
}

void HotReloadManager::unwatchAll() {
    m_fileWatcher.unwatchAll();
    m_watchedAssets.clear();
    m_pathToAsset.clear();
    m_dependencies.clear();
    m_dependents.clear();
    m_pendingReloads.clear();
    LIMBO_LOG_ASSET_DEBUG("HotReloadManager: Unwatched all assets");
}

bool HotReloadManager::isWatching(AssetId id) const {
    return m_watchedAssets.contains(id);
}

void HotReloadManager::addDependency(AssetId assetId, AssetId dependencyId) {
    if (!assetId.isValid() || !dependencyId.isValid()) {
        return;
    }

    if (assetId == dependencyId) {
        LIMBO_LOG_ASSET_WARN("HotReloadManager: Asset cannot depend on itself");
        return;
    }

    m_dependencies[assetId].insert(dependencyId);
    m_dependents[dependencyId].insert(assetId);

    LIMBO_LOG_ASSET_DEBUG("HotReloadManager: {} now depends on {}", assetId.toString(),
                          dependencyId.toString());
}

void HotReloadManager::removeDependency(AssetId assetId, AssetId dependencyId) {
    auto depIt = m_dependencies.find(assetId);
    if (depIt != m_dependencies.end()) {
        depIt->second.erase(dependencyId);
        if (depIt->second.empty()) {
            m_dependencies.erase(depIt);
        }
    }

    auto revIt = m_dependents.find(dependencyId);
    if (revIt != m_dependents.end()) {
        revIt->second.erase(assetId);
        if (revIt->second.empty()) {
            m_dependents.erase(revIt);
        }
    }
}

void HotReloadManager::clearDependencies(AssetId assetId) {
    auto depIt = m_dependencies.find(assetId);
    if (depIt != m_dependencies.end()) {
        for (AssetId depId : depIt->second) {
            auto revIt = m_dependents.find(depId);
            if (revIt != m_dependents.end()) {
                revIt->second.erase(assetId);
            }
        }
        m_dependencies.erase(depIt);
    }
}

std::vector<AssetId> HotReloadManager::getDependents(AssetId id) const {
    auto it = m_dependents.find(id);
    if (it == m_dependents.end()) {
        return {};
    }
    return {it->second.begin(), it->second.end()};
}

std::vector<AssetId> HotReloadManager::getAffectedAssets(AssetId id) const {
    std::unordered_set<AssetId> affected;
    collectAffectedAssets(id, affected);

    // Include the original asset
    affected.insert(id);

    return {affected.begin(), affected.end()};
}

std::vector<AssetId> HotReloadManager::getDependencies(AssetId id) const {
    auto it = m_dependencies.find(id);
    if (it == m_dependencies.end()) {
        return {};
    }
    return {it->second.begin(), it->second.end()};
}

void HotReloadManager::triggerReload(AssetId id) {
    if (!id.isValid()) {
        return;
    }

    // Get all affected assets (the changed asset + all dependents)
    std::unordered_set<AssetId> affected;
    affected.insert(id);
    collectAffectedAssets(id, affected);

    // Add to pending reloads
    for (AssetId affectedId : affected) {
        m_pendingReloads.insert(affectedId);
    }

    LIMBO_LOG_ASSET_INFO("HotReloadManager: Triggered reload for {} ({} assets affected)",
                         id.toString(), affected.size());

    // If not batching, process immediately
    if (!m_batchReloads) {
        processPendingReloads();
    }
}

void HotReloadManager::poll() {
    if (!m_enabled) {
        return;
    }

    // Poll file watcher for changes
    m_fileWatcher.poll();

    // Process any pending reloads
    if (!m_pendingReloads.empty()) {
        processPendingReloads();
    }
}

void HotReloadManager::processPendingReloads() {
    if (m_pendingReloads.empty()) {
        return;
    }

    if (!m_reloadHandler) {
        LIMBO_LOG_ASSET_WARN("HotReloadManager: No reload handler set");
        m_pendingReloads.clear();
        return;
    }

    // Sort assets in topological order (dependencies first)
    std::vector<AssetId> sortedAssets = topologicalSort(m_pendingReloads);

    LIMBO_LOG_ASSET_INFO("HotReloadManager: Processing {} pending reloads", sortedAssets.size());

    for (AssetId id : sortedAssets) {
        // Check before-reload callback
        if (m_beforeReloadCallback && !m_beforeReloadCallback(id)) {
            LIMBO_LOG_ASSET_DEBUG("HotReloadManager: Reload cancelled for {}", id.toString());
            continue;
        }

        // Find the path for the event
        std::filesystem::path assetPath;
        auto watchIt = m_watchedAssets.find(id);
        if (watchIt != m_watchedAssets.end() && !watchIt->second.paths.empty()) {
            assetPath = watchIt->second.paths[0];
        }

        // Perform reload
        bool success = m_reloadHandler(id);
        m_totalReloads++;

        if (!success) {
            m_failedReloads++;
            LIMBO_LOG_ASSET_ERROR("HotReloadManager: Failed to reload {}", id.toString());
        } else {
            LIMBO_LOG_ASSET_INFO("HotReloadManager: Reloaded {}", id.toString());
        }

        // Call after-reload callback
        if (m_afterReloadCallback) {
            ReloadEvent event;
            event.assetId = id;
            event.path = assetPath;
            event.success = success;
            if (!success) {
                event.error = "Reload failed";
            }
            m_afterReloadCallback(event);
        }
    }

    m_pendingReloads.clear();
}

void HotReloadManager::onFileChanged(const std::filesystem::path& path) {
    String const pathKey = path.generic_string();

    auto it = m_pathToAsset.find(pathKey);
    if (it == m_pathToAsset.end()) {
        LIMBO_LOG_ASSET_DEBUG("HotReloadManager: Changed file not mapped to any asset: {}",
                              path.string());
        return;
    }

    AssetId id = it->second;
    LIMBO_LOG_ASSET_INFO("HotReloadManager: File changed for asset {}: {}", id.toString(),
                         path.string());

    triggerReload(id);
}

void HotReloadManager::collectAffectedAssets(AssetId id,
                                             std::unordered_set<AssetId>& affected) const {
    // BFS to find all transitive dependents
    std::queue<AssetId> queue;

    auto directDeps = m_dependents.find(id);
    if (directDeps != m_dependents.end()) {
        for (AssetId depId : directDeps->second) {
            if (!affected.contains(depId)) {
                affected.insert(depId);
                queue.push(depId);
            }
        }
    }

    while (!queue.empty()) {
        AssetId current = queue.front();
        queue.pop();

        auto deps = m_dependents.find(current);
        if (deps != m_dependents.end()) {
            for (AssetId depId : deps->second) {
                if (!affected.contains(depId)) {
                    affected.insert(depId);
                    queue.push(depId);
                }
            }
        }
    }
}

std::vector<AssetId>
HotReloadManager::topologicalSort(const std::unordered_set<AssetId>& assets) const {
    // Kahn's algorithm for topological sort
    // Assets with no dependencies come first

    std::unordered_map<AssetId, i32> inDegree;
    std::unordered_map<AssetId, std::vector<AssetId>> adjList;

    // Initialize in-degree for all assets in our set
    for (AssetId id : assets) {
        inDegree[id] = 0;
    }

    // Build adjacency list and in-degree counts (only considering assets in our set)
    for (AssetId id : assets) {
        auto depIt = m_dependencies.find(id);
        if (depIt != m_dependencies.end()) {
            for (AssetId depId : depIt->second) {
                // Only count if the dependency is also in our set
                if (assets.contains(depId)) {
                    adjList[depId].push_back(id);
                    inDegree[id]++;
                }
            }
        }
    }

    // Start with assets that have no dependencies (in our set)
    std::queue<AssetId> queue;
    for (AssetId id : assets) {
        if (inDegree[id] == 0) {
            queue.push(id);
        }
    }

    std::vector<AssetId> result;
    result.reserve(assets.size());

    while (!queue.empty()) {
        AssetId current = queue.front();
        queue.pop();
        result.push_back(current);

        auto adjIt = adjList.find(current);
        if (adjIt != adjList.end()) {
            for (AssetId neighbor : adjIt->second) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }
    }

    // If we didn't process all assets, there's a cycle
    if (result.size() != assets.size()) {
        LIMBO_LOG_ASSET_WARN(
            "HotReloadManager: Dependency cycle detected, falling back to arbitrary order");
        // Just add remaining assets in arbitrary order
        for (AssetId id : assets) {
            if (std::find(result.begin(), result.end(), id) == result.end()) {
                result.push_back(id);
            }
        }
    }

    return result;
}

}  // namespace limbo
