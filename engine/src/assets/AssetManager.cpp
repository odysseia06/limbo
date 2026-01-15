#include "limbo/assets/AssetManager.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

AssetManager::~AssetManager() {
    unloadAll();
}

void AssetManager::setAssetRoot(const std::filesystem::path& root) {
    m_assetRoot = root;
    LIMBO_LOG_ASSET_INFO("Asset root set to: {}", m_assetRoot.string());
}

void AssetManager::setupHotReloadManager() {
    // Set the reload handler to call our reload method
    m_hotReloadManager.setReloadHandler([this](AssetId id) { return reload(id); });

    // Forward the after-reload callback
    m_hotReloadManager.setAfterReloadCallback([this](const ReloadEvent& event) {
        if (m_reloadCallback) {
            m_reloadCallback(event);
        }
    });

    m_hotReloadManager.setEnabled(true);
    LIMBO_LOG_ASSET_INFO("Hot reload manager initialized");
}

void AssetManager::addAssetDependency(AssetId assetId, AssetId dependencyId) {
    m_hotReloadManager.addDependency(assetId, dependencyId);
}

void AssetManager::setReloadCallback(ReloadCallback callback) {
    m_reloadCallback = std::move(callback);
}

bool AssetManager::isLoaded(AssetId id) const {
    return m_assets.contains(id);
}

void AssetManager::unload(AssetId id) {
    auto it = m_assets.find(id);
    if (it == m_assets.end()) {
        return;
    }

    // Remove from type map
    for (auto& [type, ids] : m_assetsByType) {
        auto idIt = std::find(ids.begin(), ids.end(), id);
        if (idIt != ids.end()) {
            ids.erase(idIt);
            break;
        }
    }

    // Stop watching for hot-reload before unloading
    if (m_hotReloadEnabled) {
        m_hotReloadManager.unwatchAsset(id);
    }

    // Unload and remove
    it->second->unload();
    m_assets.erase(it);
}

void AssetManager::unloadAll() {
    // Stop watching all assets for hot-reload
    if (m_hotReloadEnabled) {
        m_hotReloadManager.unwatchAll();
    }

    for (auto& [id, asset] : m_assets) {
        asset->unload();
    }
    m_assets.clear();
    m_assetsByType.clear();
}

bool AssetManager::reload(AssetId id) {
    auto it = m_assets.find(id);
    if (it == m_assets.end()) {
        return false;
    }

    LIMBO_LOG_ASSET_INFO("Reloading asset: {}", it->second->getPath().string());

    it->second->setState(AssetState::Loading);
    if (it->second->reload()) {
        it->second->setState(AssetState::Loaded);
        return true;
    } else {
        it->second->setState(AssetState::Failed);
        return false;
    }
}

void AssetManager::reloadAll() {
    for (auto& [id, asset] : m_assets) {
        reload(id);
    }
}

std::filesystem::path AssetManager::resolvePath(const std::filesystem::path& relativePath) const {
    return m_assetRoot / relativePath;
}

void AssetManager::pollHotReload() {
    if (m_hotReloadEnabled) {
        m_hotReloadManager.poll();
    }
}

void AssetManager::setHotReloadEnabled(bool enabled) {
    m_hotReloadEnabled = enabled;
    if (enabled && !m_hotReloadManager.isEnabled()) {
        setupHotReloadManager();
    }
    m_hotReloadManager.setEnabled(enabled);
}

}  // namespace limbo
