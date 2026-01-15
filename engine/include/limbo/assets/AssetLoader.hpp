#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/Asset.hpp"
#include "limbo/assets/AssetId.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace limbo {

// Forward declarations
class AssetManager;

/**
 * Callback invoked when an async asset load completes
 * @param id The asset ID
 * @param success Whether loading succeeded
 */
using AssetLoadCallback = std::function<void(AssetId id, bool success)>;

/**
 * AssetLoader - Async asset loading system
 *
 * Loads assets in the background without blocking the main thread:
 * 1. File I/O and decoding runs on worker threads
 * 2. GPU upload runs on main thread (OpenGL requirement)
 *
 * Usage:
 *   // Request async load
 *   AssetLoader::loadAsync<TextureAsset>(manager, "textures/player.png", [](AssetId id, bool ok) {
 *       if (ok) { ... asset ready ... }
 *   });
 *
 *   // In main loop:
 *   AssetLoader::processMainThreadWork();
 */
class LIMBO_API AssetLoader {
public:
    /**
     * Initialize the asset loader
     */
    static void init();

    /**
     * Shutdown the asset loader
     */
    static void shutdown();

    /**
     * Check if the loader is initialized
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * Queue an asset for async loading
     * @tparam T Asset type (must derive from Asset)
     * @param manager The asset manager
     * @param path Relative path to the asset
     * @param callback Called on main thread when loading completes
     * @return The asset ID (asset will be in Queued state)
     */
    template <typename T>
    static AssetId loadAsync(AssetManager& manager, const std::filesystem::path& path,
                             AssetLoadCallback callback = nullptr);

    /**
     * Process main thread work (GPU uploads)
     * Call this once per frame from the main thread
     * @return Number of assets processed
     */
    static usize processMainThreadWork();

    /**
     * Check if any async loads are in progress
     */
    [[nodiscard]] static bool isLoading();

    /**
     * Get the number of pending async loads
     */
    [[nodiscard]] static usize getPendingCount();

    /**
     * Wait for all async loads to complete (blocks)
     */
    static void waitAll();

private:
    // Internal load request
    struct LoadRequest {
        AssetId id;
        Shared<Asset> asset;
        AssetManager* manager;
        AssetLoadCallback callback;
    };

    // GPU upload request (queued for main thread)
    struct GPUUploadRequest {
        AssetId id;
        Shared<Asset> asset;
        AssetManager* manager;
        AssetLoadCallback callback;
        std::vector<u8> data;  // Decoded data ready for GPU upload
    };

    static void ioWorker(LoadRequest request);

    static inline std::mutex s_mutex;
    static inline std::queue<GPUUploadRequest> s_gpuQueue;
    static inline std::atomic<usize> s_pendingCount{0};
    static inline std::atomic<bool> s_initialized{false};
};

}  // namespace limbo

// Template implementation - must be in header
#include "limbo/assets/AssetManager.hpp"
#include "limbo/core/ThreadPool.hpp"

namespace limbo {

template <typename T>
AssetId AssetLoader::loadAsync(AssetManager& manager, const std::filesystem::path& path,
                               AssetLoadCallback callback) {
    static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");

    // Generate ID from path
    String const pathStr = path.generic_string();
    AssetId const id(pathStr);

    // Check if loader and thread pool are initialized
    if (!isInitialized() || !ThreadPool::isInitialized()) {
        if (callback) {
            callback(id, false);
        }
        return id;
    }

    // Check if already loaded or loading
    auto existing = manager.get<T>(path);
    if (existing) {
        // Already loaded or in progress
        if (existing->isLoaded() && callback) {
            callback(id, true);
        }
        return id;
    }

    // Create new asset
    auto asset = make_shared<T>();
    asset->setId(id);
    asset->setPath(manager.resolvePath(path));
    asset->setState(AssetState::Queued);

    // Register with manager immediately so get() can find it
    manager.registerAsset<T>(asset);

    // Increment pending count
    s_pendingCount.fetch_add(1);

    // Create load request
    LoadRequest request;
    request.id = id;
    request.asset = asset;
    request.manager = &manager;
    request.callback = std::move(callback);

    // Submit to thread pool
    ThreadPool::submit([request = std::move(request)]() mutable { ioWorker(std::move(request)); });

    return id;
}

}  // namespace limbo
