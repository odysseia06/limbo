#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace limbo {

/**
 * Thread pool for executing work in parallel
 *
 * Thread safety rules (IMPORTANT):
 * - NO OpenGL/GLFW/ImGui calls from worker threads
 * - NO entity creation/destruction from worker threads
 * - SAFE: File I/O, image decoding, audio decoding, math, parsing
 *
 * For results that need to affect the main thread, use MainThreadQueue
 * to enqueue work to be processed on the main thread.
 */
class LIMBO_API ThreadPool {
public:
    using JobFunction = std::function<void()>;

    /**
     * Initialize the thread pool
     * @param numThreads Number of worker threads (0 = hardware_concurrency - 1)
     */
    static void init(u32 numThreads = 0);

    /**
     * Shutdown the thread pool, waiting for all jobs to complete
     */
    static void shutdown();

    /**
     * Check if the thread pool is initialized
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * Submit a job to be executed on a worker thread
     * @param job The function to execute
     * @return Future that completes when the job is done
     */
    static std::future<void> submit(JobFunction job);

    /**
     * Submit a job that returns a value
     * @tparam T Return type
     * @param job The function to execute
     * @return Future containing the result (exception set if pool not initialized)
     */
    template <typename T>
    static std::future<T> submit(std::function<T()> job) {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();

        if (!s_running.load()) {
            promise->set_exception(
                std::make_exception_ptr(std::runtime_error("ThreadPool not initialized")));
            return future;
        }

        submit([promise = std::move(promise), job = std::move(job)]() {
            try {
                promise->set_value(job());
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
    }

    /**
     * Wait for all currently queued jobs to complete
     */
    static void waitAll();

    /**
     * Check if the current thread is the main thread
     */
    [[nodiscard]] static bool isMainThread();

    /**
     * Get the number of worker threads
     */
    [[nodiscard]] static u32 getWorkerCount();

    /**
     * Get the number of pending jobs in the queue
     */
    [[nodiscard]] static usize getPendingJobCount();

private:
    ThreadPool() = default;
    ~ThreadPool() = default;

    static void workerLoop();

    static inline std::vector<std::thread> s_workers;
    static inline std::queue<JobFunction> s_jobQueue;
    static inline std::mutex s_queueMutex;
    static inline std::condition_variable s_condition;
    static inline std::condition_variable s_doneCondition;
    static inline std::atomic<bool> s_running{false};
    static inline std::atomic<u32> s_activeJobs{0};
    static inline std::thread::id s_mainThreadId;
};

}  // namespace limbo
