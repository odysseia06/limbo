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

        bool success = false;

        if (!request.ioSucceeded) {
            // IO failed, just invoke callback
            success = false;
        } else if (request.asset->supportsAsyncLoad()) {
            // Asset supports IO/GPU split - do GPU upload only
            request.asset->setState(AssetState::LoadingGPU);
            success = request.asset->uploadGPU();
        } else {
            // Fallback: asset doesn't support async, do full load on main thread
            request.asset->setState(AssetState::LoadingGPU);
            success = request.asset->load();
        }

        if (success) {
            request.asset->setState(AssetState::Loaded);
            LIMBO_LOG_ASSET_DEBUG("AssetLoader: Loaded {}", request.asset->getPath().string());
        } else {
            request.asset->setState(AssetState::Failed);
            LIMBO_LOG_ASSET_ERROR("AssetLoader: Failed to load {}",
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
    // This runs on a worker thread - perform file I/O and decoding
    request.asset->setState(AssetState::LoadingIO);

    bool ioSuccess = false;
    if (request.asset->supportsAsyncLoad()) {
        // Use the proper IO/GPU split
        ioSuccess = request.asset->loadIO();
    } else {
        // Fallback: asset doesn't support async, will do full load on main thread
        ioSuccess = true;
    }

    if (!ioSuccess) {
        // IO failed, invoke callback and decrement counter
        request.asset->setState(AssetState::Failed);
        if (request.callback) {
            // Queue callback for main thread
            GPUUploadRequest failRequest;
            failRequest.id = request.id;
            failRequest.asset = request.asset;
            failRequest.manager = request.manager;
            failRequest.callback = std::move(request.callback);
            failRequest.ioSucceeded = false;

            std::lock_guard<std::mutex> lock(s_mutex);
            s_gpuQueue.push(std::move(failRequest));
        } else {
            s_pendingCount.fetch_sub(1);
        }
        return;
    }

    // Queue GPU upload for main thread
    GPUUploadRequest gpuRequest;
    gpuRequest.id = request.id;
    gpuRequest.asset = request.asset;
    gpuRequest.manager = request.manager;
    gpuRequest.callback = std::move(request.callback);
    gpuRequest.ioSucceeded = true;

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_gpuQueue.push(std::move(gpuRequest));
    }
}

}  // namespace limbo
