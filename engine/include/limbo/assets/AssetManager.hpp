#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/Asset.hpp"
#include "limbo/assets/AssetId.hpp"
#include "limbo/assets/FileWatcher.hpp"

#include <filesystem>
#include <unordered_map>
#include <typeindex>

namespace limbo {

// Forward declarations
class TextureAsset;
class ShaderAsset;

/**
 * AssetManager - Centralized asset loading and caching
 *
 * The AssetManager handles loading assets from disk, caching them for
 * reuse, and supporting hot-reloading during development.
 *
 * Usage:
 *   AssetManager manager;
 *   manager.setAssetRoot("assets/");
 *
 *   auto texture = manager.load<TextureAsset>("textures/player.png");
 *   auto shader = manager.load<ShaderAsset>("shaders/basic");
 */
class LIMBO_API AssetManager {
public:
    AssetManager() = default;
    ~AssetManager();

    // Non-copyable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    /**
     * Set the root directory for asset loading
     * All asset paths will be relative to this directory
     */
    void setAssetRoot(const std::filesystem::path& root);

    /**
     * Get the asset root directory
     */
    [[nodiscard]] const std::filesystem::path& getAssetRoot() const { return m_assetRoot; }

    /**
     * Load an asset by path
     * If the asset is already loaded, returns the cached version
     * @tparam T Asset type (must derive from Asset)
     * @param path Relative path to the asset
     * @return Shared pointer to the asset
     */
    template <typename T>
    Shared<T> load(const std::filesystem::path& path);

    /**
     * Get a previously loaded asset by ID
     * @tparam T Asset type
     * @param id The asset's ID
     * @return Shared pointer to the asset, or nullptr if not found
     */
    template <typename T>
    Shared<T> get(AssetId id);

    /**
     * Get a previously loaded asset by path
     * @tparam T Asset type
     * @param path The asset's path
     * @return Shared pointer to the asset, or nullptr if not found
     */
    template <typename T>
    Shared<T> get(const std::filesystem::path& path);

    /**
     * Check if an asset is loaded
     */
    [[nodiscard]] bool isLoaded(AssetId id) const;

    /**
     * Unload a specific asset
     */
    void unload(AssetId id);

    /**
     * Unload all assets of a specific type
     */
    template <typename T>
    void unloadAll();

    /**
     * Unload all assets
     */
    void unloadAll();

    /**
     * Reload a specific asset (for hot-reloading)
     * @return true if reload succeeded
     */
    bool reload(AssetId id);

    /**
     * Reload all assets (for hot-reloading)
     */
    void reloadAll();

    /**
     * Get the number of loaded assets
     */
    [[nodiscard]] usize assetCount() const { return m_assets.size(); }

    /**
     * Resolve a relative path to an absolute path using the asset root
     */
    [[nodiscard]] std::filesystem::path
    resolvePath(const std::filesystem::path& relativePath) const;

    /**
     * Enable or disable hot-reloading
     * When enabled, assets will be automatically reloaded when their files change
     */
    void setHotReloadEnabled(bool enabled) { m_hotReloadEnabled = enabled; }

    /**
     * Check if hot-reloading is enabled
     */
    [[nodiscard]] bool isHotReloadEnabled() const { return m_hotReloadEnabled; }

    /**
     * Poll for file changes and trigger hot-reloads
     * Call this regularly (e.g., once per frame) when hot-reload is enabled
     */
    void pollHotReload();

    /**
     * Get the file watcher (for advanced configuration)
     */
    [[nodiscard]] FileWatcher& getFileWatcher() { return m_fileWatcher; }

private:
    std::filesystem::path m_assetRoot = "assets";
    std::unordered_map<AssetId, Shared<Asset>> m_assets;
    std::unordered_map<std::type_index, std::vector<AssetId>> m_assetsByType;

    FileWatcher m_fileWatcher;
    bool m_hotReloadEnabled = false;
};

// Template implementations
template <typename T>
Shared<T> AssetManager::load(const std::filesystem::path& path) {
    static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");

    // Generate ID from path
    String pathStr = path.generic_string();
    AssetId id(pathStr);

    // Check if already loaded
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return std::dynamic_pointer_cast<T>(it->second);
    }

    // Create new asset
    auto asset = make_shared<T>();
    asset->setId(id);
    asset->setPath(resolvePath(path));
    asset->setState(AssetState::Loading);

    // Try to load
    if (asset->load()) {
        asset->setState(AssetState::Loaded);

        // Register file dependencies for hot-reload
        if (m_hotReloadEnabled) {
            for (const auto& depPath : asset->getDependencies()) {
                m_fileWatcher.watch(depPath,
                                    [this, id](const std::filesystem::path&) { reload(id); });
            }
        }
    } else {
        asset->setState(AssetState::Failed);
    }

    // Cache it
    m_assets[id] = asset;
    m_assetsByType[std::type_index(typeid(T))].push_back(id);

    return asset;
}

template <typename T>
Shared<T> AssetManager::get(AssetId id) {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return std::dynamic_pointer_cast<T>(it->second);
    }
    return nullptr;
}

template <typename T>
Shared<T> AssetManager::get(const std::filesystem::path& path) {
    String pathStr = path.generic_string();
    AssetId id(pathStr);
    return get<T>(id);
}

template <typename T>
void AssetManager::unloadAll() {
    auto typeIt = m_assetsByType.find(std::type_index(typeid(T)));
    if (typeIt == m_assetsByType.end()) {
        return;
    }

    for (AssetId id : typeIt->second) {
        auto it = m_assets.find(id);
        if (it != m_assets.end()) {
            it->second->unload();
            m_assets.erase(it);
        }
    }

    m_assetsByType.erase(typeIt);
}

}  // namespace limbo
