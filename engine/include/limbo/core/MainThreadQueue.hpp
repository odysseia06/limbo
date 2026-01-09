#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <functional>
#include <mutex>
#include <queue>

namespace limbo {

/**
 * MainThreadQueue - Queue for executing work on the main thread
 *
 * Use this to defer OpenGL/GLFW/ImGui calls from worker threads.
 * Call processAll() once per frame from the main thread.
 *
 * Example:
 *   // From a worker thread:
 *   MainThreadQueue::enqueue([texture, pixels]() {
 *       texture->upload(pixels);  // OpenGL call - safe on main thread
 *   });
 *
 *   // In main loop:
 *   MainThreadQueue::processAll();
 */
class LIMBO_API MainThreadQueue {
public:
    using Task = std::function<void()>;

    /**
     * Enqueue a task to be executed on the main thread
     * Thread-safe: can be called from any thread
     * @param task The function to execute
     */
    static void enqueue(Task task);

    /**
     * Process all queued tasks
     * Must be called from the main thread, typically once per frame
     * @return Number of tasks processed
     */
    static usize processAll();

    /**
     * Check if there are pending tasks
     */
    [[nodiscard]] static bool hasPendingTasks();

    /**
     * Get the number of pending tasks
     */
    [[nodiscard]] static usize getPendingCount();

    /**
     * Clear all pending tasks without executing them
     */
    static void clear();

private:
    static inline std::queue<Task> s_taskQueue;
    static inline std::mutex s_mutex;
};

}  // namespace limbo
