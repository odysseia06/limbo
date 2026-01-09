#include "limbo/assets/AssetLoader.hpp"

#include "limbo/assets/AssetManager.hpp"
#include "limbo/core/MainThreadQueue.hpp"
#include "limbo/core/ThreadPool.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo {

void AssetLoader::init() {
    if (s_initialized.load()) {
        LIMBO_LOG_ASSET_WARN("AssetLoader: Already initialized");
        return;
    }
    s_initialized.store(true);
    LIMBO_LOG_ASSET_DEBUG("AssetLoader: Initialized");
}

void AssetLoader::shutdown() {
    if (!s_initialized.load()) {
        return;
    }

    // Wait for pending loads
    waitAll();

    // Clear GPU queue
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        while (!s_gpuQueue.empty()) {
            s_gpuQueue.pop();
        }
    }

    s_initialized.store(false);
    LIMBO_LOG_ASSET_DEBUG("AssetLoader: Shutdown");
}

bool AssetLoader::isInitialized() {
    return s_initialized.load();
}

usize AssetLoader::processMainThreadWork() {
    if (!ThreadPool::isMainThread()) {
        LIMBO_LOG_ASSET_ERROR(
            "AssetLoader::processMainThreadWork() must be called from main thread");
        return 0;
    }

    // Get all pending GPU uploads
    std::queue<GPUUploadRequest> uploads;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        std::swap(uploads, s_gpuQueue);
    }

    usize count = 0;
    while (!uploads.empty()) {
        auto& request = uploads.front();

        // Update asset state
        request.asset->setState(AssetState::LoadingGPU);

        // Perform GPU upload (the asset's load method handles this)
        bool success = request.asset->load();

        if (success) {
            request.asset->setState(AssetState::Loaded);
            LIMBO_LOG_ASSET_DEBUG("AssetLoader: Loaded {}", request.asset->getPath().string());
        } else {
            request.asset->setState(AssetState::Failed);
            LIMBO_LOG_ASSET_ERROR("AssetLoader: Failed to upload {} to GPU",
                                  request.asset->getPath().string());
        }

        // Invoke callback on main thread
        if (request.callback) {
            request.callback(request.id, success);
        }

        s_pendingCount.fetch_sub(1);
        uploads.pop();
        ++count;
    }

    return count;
}

bool AssetLoader::isLoading() {
    return s_pendingCount.load() > 0;
}

usize AssetLoader::getPendingCount() {
    return s_pendingCount.load();
}

void AssetLoader::waitAll() {
    // Process main thread work until all loads complete
    while (isLoading()) {
        if (ThreadPool::isMainThread()) {
            processMainThreadWork();
        }
        std::this_thread::yield();
    }
}

void AssetLoader::ioWorker(LoadRequest request) {
    // This runs on a worker thread
    // Perform file I/O and decoding
    request.asset->setState(AssetState::LoadingIO);

    // Read and decode the asset data
    // For now, we queue the GPU upload which will call the asset's load() method
    // In a more complete implementation, we'd have separate loadIO() and uploadGPU() methods

    // Queue GPU upload for main thread
    GPUUploadRequest gpuRequest;
    gpuRequest.id = request.id;
    gpuRequest.asset = request.asset;
    gpuRequest.manager = request.manager;
    gpuRequest.callback = std::move(request.callback);

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_gpuQueue.push(std::move(gpuRequest));
    }
}

}  // namespace limbo
