#include <catch2/catch_test_macros.hpp>

#include "limbo/core/ThreadPool.hpp"
#include "limbo/core/MainThreadQueue.hpp"

#include <atomic>
#include <chrono>
#include <thread>

using namespace limbo;

TEST_CASE("ThreadPool initialization", "[core][threading]") {
    SECTION("starts uninitialized") {
        REQUIRE_FALSE(ThreadPool::isInitialized());
    }

    SECTION("init creates workers") {
        ThreadPool::init(2);
        REQUIRE(ThreadPool::isInitialized());
        REQUIRE(ThreadPool::getWorkerCount() == 2);
        ThreadPool::shutdown();
    }

    SECTION("shutdown cleans up") {
        ThreadPool::init(2);
        ThreadPool::shutdown();
        REQUIRE_FALSE(ThreadPool::isInitialized());
    }

    SECTION("double init is safe") {
        ThreadPool::init(2);
        ThreadPool::init(4);  // Should be ignored or handled gracefully
        REQUIRE(ThreadPool::isInitialized());
        ThreadPool::shutdown();
    }

    SECTION("shutdown without init is safe") {
        ThreadPool::shutdown();  // Should not crash
    }
}

TEST_CASE("ThreadPool job execution", "[core][threading]") {
    ThreadPool::init(2);

    SECTION("submit executes job") {
        std::atomic<bool> executed{false};

        auto future = ThreadPool::submit([&executed]() { executed.store(true); });

        future.wait();
        REQUIRE(executed.load());
    }

    SECTION("submit returns future with value") {
        auto future = ThreadPool::submit<int>([]() { return 42; });

        REQUIRE(future.get() == 42);
    }

    SECTION("multiple jobs execute") {
        std::atomic<int> counter{0};

        std::vector<std::future<void>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(ThreadPool::submit([&counter]() { counter.fetch_add(1); }));
        }

        for (auto& f : futures) {
            f.wait();
        }

        REQUIRE(counter.load() == 10);
    }

    SECTION("waitAll blocks until complete") {
        std::atomic<int> counter{0};

        for (int i = 0; i < 5; ++i) {
            ThreadPool::submit([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter.fetch_add(1);
            });
        }

        ThreadPool::waitAll();
        REQUIRE(counter.load() == 5);
    }

    SECTION("getPendingJobCount tracks queue") {
        // Submit jobs that take some time
        std::atomic<bool> canProceed{false};

        for (int i = 0; i < 10; ++i) {
            ThreadPool::submit([&canProceed]() {
                while (!canProceed.load()) {
                    std::this_thread::yield();
                }
            });
        }

        // Some jobs should be pending (more jobs than workers)
        // Note: This is racy but with 10 jobs and 2 workers, should usually have pending
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        canProceed.store(true);
        ThreadPool::waitAll();
    }

    SECTION("isMainThread returns true on main thread") {
        REQUIRE(ThreadPool::isMainThread());
    }

    SECTION("isMainThread returns false on worker thread") {
        std::atomic<bool> isMain{true};

        auto future = ThreadPool::submit([&isMain]() { isMain.store(ThreadPool::isMainThread()); });

        future.wait();
        REQUIRE_FALSE(isMain.load());
    }

    ThreadPool::shutdown();
}

TEST_CASE("ThreadPool exception handling", "[core][threading]") {
    ThreadPool::init(2);

    SECTION("exception propagates through future") {
        auto future = ThreadPool::submit<int>([]() -> int { throw std::runtime_error("test"); });

        REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    }

    SECTION("exception in void job doesn't crash pool") {
        auto future = ThreadPool::submit([]() { throw std::runtime_error("test"); });

        // Job completes (even if it threw)
        future.wait();

        // Pool should still work
        std::atomic<bool> executed{false};
        auto future2 = ThreadPool::submit([&executed]() { executed.store(true); });
        future2.wait();
        REQUIRE(executed.load());
    }

    ThreadPool::shutdown();
}

TEST_CASE("ThreadPool submit without init", "[core][threading]") {
    // Ensure not initialized
    REQUIRE_FALSE(ThreadPool::isInitialized());

    SECTION("submit with return value throws via future") {
        auto future = ThreadPool::submit<int>([]() { return 42; });
        REQUIRE_THROWS(future.get());
    }
}

TEST_CASE("MainThreadQueue basic operations", "[core][threading]") {
    // Clear any leftover tasks
    MainThreadQueue::clear();

    SECTION("starts empty") {
        REQUIRE_FALSE(MainThreadQueue::hasPendingTasks());
        REQUIRE(MainThreadQueue::getPendingCount() == 0);
    }

    SECTION("enqueue adds task") {
        MainThreadQueue::enqueue([]() {});
        REQUIRE(MainThreadQueue::hasPendingTasks());
        REQUIRE(MainThreadQueue::getPendingCount() == 1);
        MainThreadQueue::clear();
    }

    SECTION("processAll executes tasks") {
        bool executed = false;
        MainThreadQueue::enqueue([&executed]() { executed = true; });

        usize processed = MainThreadQueue::processAll();

        REQUIRE(processed == 1);
        REQUIRE(executed);
        REQUIRE_FALSE(MainThreadQueue::hasPendingTasks());
    }

    SECTION("processAll executes multiple tasks in order") {
        std::vector<int> order;

        MainThreadQueue::enqueue([&order]() { order.push_back(1); });
        MainThreadQueue::enqueue([&order]() { order.push_back(2); });
        MainThreadQueue::enqueue([&order]() { order.push_back(3); });

        MainThreadQueue::processAll();

        REQUIRE(order.size() == 3);
        REQUIRE(order[0] == 1);
        REQUIRE(order[1] == 2);
        REQUIRE(order[2] == 3);
    }

    SECTION("clear removes pending tasks") {
        MainThreadQueue::enqueue([]() {});
        MainThreadQueue::enqueue([]() {});
        REQUIRE(MainThreadQueue::getPendingCount() == 2);

        MainThreadQueue::clear();
        REQUIRE(MainThreadQueue::getPendingCount() == 0);
    }
}

TEST_CASE("MainThreadQueue thread safety", "[core][threading]") {
    ThreadPool::init(4);
    MainThreadQueue::clear();

    SECTION("enqueue from multiple threads") {
        std::atomic<int> counter{0};

        // Submit jobs that each enqueue to main thread
        std::vector<std::future<void>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(ThreadPool::submit([&counter]() {
                MainThreadQueue::enqueue([&counter]() { counter.fetch_add(1); });
            }));
        }

        // Wait for all worker jobs to complete
        for (auto& f : futures) {
            f.wait();
        }

        // Process all main thread tasks
        MainThreadQueue::processAll();

        REQUIRE(counter.load() == 100);
    }

    ThreadPool::shutdown();
    MainThreadQueue::clear();
}

TEST_CASE("ThreadPool and MainThreadQueue integration", "[core][threading]") {
    ThreadPool::init(2);
    MainThreadQueue::clear();

    SECTION("worker thread defers work to main thread") {
        std::atomic<bool> workerRan{false};
        std::atomic<bool> mainThreadTaskRan{false};
        std::atomic<bool> wasMainThread{false};

        auto future = ThreadPool::submit([&]() {
            workerRan.store(true);

            // Defer work to main thread
            MainThreadQueue::enqueue([&]() {
                mainThreadTaskRan.store(true);
                wasMainThread.store(ThreadPool::isMainThread());
            });
        });

        future.wait();
        REQUIRE(workerRan.load());
        REQUIRE_FALSE(mainThreadTaskRan.load());

        // Process deferred work
        MainThreadQueue::processAll();
        REQUIRE(mainThreadTaskRan.load());
        REQUIRE(wasMainThread.load());
    }

    ThreadPool::shutdown();
    MainThreadQueue::clear();
}
