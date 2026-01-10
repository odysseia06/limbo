#include "limbo/scripting/ScriptHotReloadManager.hpp"
#include "limbo/scripting/ScriptComponent.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo {

void ScriptHotReloadManager::watchScript(const std::filesystem::path& path,
                                         World::EntityId entityId) {
    String const pathStr = path.string();

    // Add entity to the set for this script
    auto& entities = m_scriptToEntities[pathStr];
    entities.insert(entityId);

    // If this is the first entity using this script, start watching
    if (entities.size() == 1) {
        m_fileWatcher.watch(path, [this](const std::filesystem::path& changedPath) {
            onScriptChanged(changedPath);
        });
        LIMBO_LOG_SCRIPT_DEBUG("ScriptHotReloadManager: Watching {}", pathStr);
    }
}

void ScriptHotReloadManager::unwatchScript(const std::filesystem::path& path,
                                           World::EntityId entityId) {
    String const pathStr = path.string();

    auto it = m_scriptToEntities.find(pathStr);
    if (it == m_scriptToEntities.end()) {
        return;
    }

    // Remove entity from the set
    it->second.erase(entityId);

    // If no more entities use this script, stop watching
    if (it->second.empty()) {
        m_fileWatcher.unwatch(path);
        m_scriptToEntities.erase(it);
        LIMBO_LOG_SCRIPT_DEBUG("ScriptHotReloadManager: Unwatched {}", pathStr);
    }
}

void ScriptHotReloadManager::unwatchEntity(World::EntityId entityId) {
    // Find all scripts this entity uses and remove it
    std::vector<String> emptyScripts;

    for (auto& [pathStr, entities] : m_scriptToEntities) {
        entities.erase(entityId);
        if (entities.empty()) {
            emptyScripts.push_back(pathStr);
        }
    }

    // Remove scripts with no entities
    for (const auto& pathStr : emptyScripts) {
        m_fileWatcher.unwatch(pathStr);
        m_scriptToEntities.erase(pathStr);
        LIMBO_LOG_SCRIPT_DEBUG("ScriptHotReloadManager: Unwatched {}", pathStr);
    }
}

void ScriptHotReloadManager::unwatchAll() {
    m_fileWatcher.unwatchAll();
    m_scriptToEntities.clear();
    m_pendingReloads.clear();
    LIMBO_LOG_SCRIPT_DEBUG("ScriptHotReloadManager: Unwatched all scripts");
}

bool ScriptHotReloadManager::isWatching(const std::filesystem::path& path) const {
    return m_scriptToEntities.find(path.string()) != m_scriptToEntities.end();
}

void ScriptHotReloadManager::poll(World& world) {
    if (!m_enabled) {
        return;
    }

    // Check for file changes
    m_fileWatcher.poll();

    // Process pending reloads
    if (m_pendingReloads.empty()) {
        return;
    }

    // Copy and clear pending reloads to avoid issues if reload triggers more changes
    auto pending = std::move(m_pendingReloads);
    m_pendingReloads.clear();

    for (const auto& pathStr : pending) {
        reloadScript(world, pathStr);
    }
}

void ScriptHotReloadManager::triggerReload(const std::filesystem::path& path) {
    m_pendingReloads.insert(path.string());
}

void ScriptHotReloadManager::onScriptChanged(const std::filesystem::path& path) {
    if (!m_enabled) {
        return;
    }

    String const pathStr = path.string();
    LIMBO_LOG_SCRIPT_INFO("Script changed: {}", pathStr);
    m_pendingReloads.insert(pathStr);
}

void ScriptHotReloadManager::reloadScript(World& world, const std::filesystem::path& path) {
    String const pathStr = path.string();

    auto it = m_scriptToEntities.find(pathStr);
    if (it == m_scriptToEntities.end()) {
        return;
    }

    LIMBO_LOG_SCRIPT_INFO("Reloading script: {}", pathStr);
    m_totalReloads++;

    bool anyFailed = false;

    // Reset and re-initialize all entities using this script
    for (World::EntityId entityId : it->second) {
        if (!world.hasComponent<ScriptComponent>(entityId)) {
            continue;
        }

        auto& script = world.getComponent<ScriptComponent>(entityId);

        // Call onBeforeReload if it exists (script can save state)
        if (script.environment.valid()) {
            sol::protected_function onBeforeReload = script.environment["onBeforeReload"];
            if (onBeforeReload.valid()) {
                try {
                    auto result = onBeforeReload();
                    if (!result.valid()) {
                        sol::error const err = result;
                        LIMBO_LOG_SCRIPT_WARN("Script onBeforeReload error: {}", err.what());
                    }
                } catch (const sol::error& e) {
                    LIMBO_LOG_SCRIPT_WARN("Script onBeforeReload exception: {}", e.what());
                }
            }
        }

        // Reset script state - it will be re-initialized on next update
        script.initialized = false;
        script.started = false;
        script.enabled = true;  // Re-enable in case it was disabled due to error

        // Clear cached functions
        script.onStart = sol::nil;
        script.onUpdate = sol::nil;
        script.onDestroy = sol::nil;
        script.onCollisionBegin = sol::nil;
        script.onCollisionEnd = sol::nil;
        script.onTriggerEnter = sol::nil;
        script.onTriggerExit = sol::nil;

        // Note: The script will be re-initialized by ScriptSystem::update()
        // on the next frame, which will call initializeScript() and then onStart()

        LIMBO_LOG_SCRIPT_DEBUG("Reset script state for entity {}", static_cast<u32>(entityId));
    }

    if (anyFailed) {
        m_failedReloads++;
    }

    // Invoke callback
    if (m_reloadCallback) {
        m_reloadCallback(path, !anyFailed);
    }
}

}  // namespace limbo
