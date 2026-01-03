#include "limbo/assets/AssetManager.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

AssetManager::~AssetManager() {
    unloadAll();
}

void AssetManager::setAssetRoot(const std::filesystem::path& root) {
    m_assetRoot = root;
    spdlog::info("Asset root set to: {}", m_assetRoot.string());
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

    // Unload and remove
    it->second->unload();
    m_assets.erase(it);
}

void AssetManager::unloadAll() {
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

    spdlog::info("Reloading asset: {}", it->second->getPath().string());

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
        m_fileWatcher.poll();
    }
}

}  // namespace limbo
