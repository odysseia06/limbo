#include "limbo/core/ThreadPool.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

void ThreadPool::init(u32 numThreads) {
    if (s_running.load()) {
        LIMBO_LOG_CORE_WARN("ThreadPool: Already initialized");
        return;
    }

    // Store main thread ID
    s_mainThreadId = std::this_thread::get_id();

    // Determine thread count
    if (numThreads == 0) {
        u32 hwThreads = std::thread::hardware_concurrency();
        // Leave at least one core for main thread, minimum 1 worker
        numThreads = hwThreads > 1 ? hwThreads - 1 : 1;
    }

    s_running.store(true);

    // Create worker threads
    s_workers.reserve(numThreads);
    for (u32 i = 0; i < numThreads; ++i) {
        s_workers.emplace_back(workerLoop);
    }

    LIMBO_LOG_CORE_DEBUG("ThreadPool: Initialized with {} worker threads", numThreads);
}

void ThreadPool::shutdown() {
    if (!s_running.load()) {
        return;
    }

    LIMBO_LOG_CORE_DEBUG("ThreadPool: Shutting down...");

    // Signal shutdown
    s_running.store(false);
    s_condition.notify_all();

    // Wait for all workers to finish
    for (auto& worker : s_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    s_workers.clear();

    // Clear any remaining jobs
    {
        std::lock_guard<std::mutex> lock(s_queueMutex);
        while (!s_jobQueue.empty()) {
            s_jobQueue.pop();
        }
    }

    LIMBO_LOG_CORE_DEBUG("ThreadPool: Shutdown complete");
}

bool ThreadPool::isInitialized() {
    return s_running.load();
}

std::future<void> ThreadPool::submit(JobFunction job) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    {
        std::lock_guard<std::mutex> lock(s_queueMutex);
        s_jobQueue.push([promise = std::move(promise), job = std::move(job)]() {
            try {
                job();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
    }

    s_condition.notify_one();
    return future;
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(s_queueMutex);
    s_doneCondition.wait(lock, [] { return s_jobQueue.empty() && s_activeJobs.load() == 0; });
}

bool ThreadPool::isMainThread() {
    return std::this_thread::get_id() == s_mainThreadId;
}

u32 ThreadPool::getWorkerCount() {
    return static_cast<u32>(s_workers.size());
}

usize ThreadPool::getPendingJobCount() {
    std::lock_guard<std::mutex> lock(s_queueMutex);
    return s_jobQueue.size();
}

void ThreadPool::workerLoop() {
    while (true) {
        JobFunction job;

        {
            std::unique_lock<std::mutex> lock(s_queueMutex);

            // Wait for a job or shutdown signal
            s_condition.wait(lock, [] { return !s_jobQueue.empty() || !s_running.load(); });

            // Check for shutdown
            if (!s_running.load() && s_jobQueue.empty()) {
                return;
            }

            // Get the job
            if (!s_jobQueue.empty()) {
                job = std::move(s_jobQueue.front());
                s_jobQueue.pop();
                s_activeJobs.fetch_add(1);
            }
        }

        // Execute the job outside the lock
        if (job) {
            job();
            s_activeJobs.fetch_sub(1);

            // Notify waitAll if queue is empty and no active jobs
            {
                std::lock_guard<std::mutex> lock(s_queueMutex);
                if (s_jobQueue.empty() && s_activeJobs.load() == 0) {
                    s_doneCondition.notify_all();
                }
            }
        }
    }
}

}  // namespace limbo
