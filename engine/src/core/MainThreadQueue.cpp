#include "limbo/core/MainThreadQueue.hpp"

#include "limbo/core/ThreadPool.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo {

void MainThreadQueue::enqueue(Task task) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_taskQueue.push(std::move(task));
}

usize MainThreadQueue::processAll() {
    // Verify we're on the main thread
    if (ThreadPool::isInitialized() && !ThreadPool::isMainThread()) {
        LIMBO_LOG_CORE_ERROR("MainThreadQueue::processAll() called from non-main thread!");
        return 0;
    }

    // Swap out the queue to minimize lock time
    std::queue<Task> tasksToProcess;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        std::swap(tasksToProcess, s_taskQueue);
    }

    usize count = 0;
    while (!tasksToProcess.empty()) {
        auto& task = tasksToProcess.front();
        try {
            task();
        } catch (const std::exception& e) {
            LIMBO_LOG_CORE_ERROR("MainThreadQueue: Task threw exception: {}", e.what());
        } catch (...) {
            LIMBO_LOG_CORE_ERROR("MainThreadQueue: Task threw unknown exception");
        }
        tasksToProcess.pop();
        ++count;
    }

    return count;
}

bool MainThreadQueue::hasPendingTasks() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return !s_taskQueue.empty();
}

usize MainThreadQueue::getPendingCount() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_taskQueue.size();
}

void MainThreadQueue::clear() {
    std::lock_guard<std::mutex> lock(s_mutex);
    while (!s_taskQueue.empty()) {
        s_taskQueue.pop();
    }
}

}  // namespace limbo
