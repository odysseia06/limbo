#include "limbo/runtime/Application.hpp"
#include "limbo/core/Time.hpp"
#include "limbo/core/FrameAllocator.hpp"
#include "limbo/core/ThreadPool.hpp"
#include "limbo/core/MainThreadQueue.hpp"
#include "limbo/assets/AssetLoader.hpp"
#include "limbo/platform/Input.hpp"
#include "limbo/input/InputManager.hpp"
#include "limbo/debug/Profiler.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

Application* Application::s_instance = nullptr;

Application::~Application() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

Result<void> Application::init(const ApplicationConfig& config) {
    if (s_instance != nullptr) {
        return unexpected<String>("Application already initialized");
    }
    s_instance = this;

    spdlog::info("Initializing application: {}", config.appName);

    // Initialize platform
    if (!platform::init()) {
        return unexpected<String>("Failed to initialize platform");
    }

    // Create window
    auto windowResult = Window::create(config.window);
    if (!windowResult) {
        return unexpected<String>(windowResult.error());
    }
    m_window = make_unique<Window>(std::move(*windowResult));

    // Initialize input system
    Input::init(*m_window);
    InputManager::init();

    // Initialize time system
    Time::init(config.time);

    // Initialize profiler
    profiler::Profiler::init();

    // Initialize frame allocator (1MB default)
    frame::init();

    // Initialize thread pool
    ThreadPool::init();

    // Initialize async asset loader
    AssetLoader::init();

    onInit();

    // Initialize systems after user has added them in onInit
    m_systems.init(m_world);

    spdlog::info("Application initialized successfully");
    return {};
}

void Application::run() {
    m_running = true;

    spdlog::info("Entering main loop");

    while (m_running && !m_window->shouldClose()) {
        // Begin frame timing and profiling
        Time::beginFrame();
        profiler::Profiler::beginFrame();
        frame::reset();  // Reset frame allocator for new frame

        f32 const deltaTime = Time::getDeltaTime();
        f32 const fixedDeltaTime = Time::getFixedDeltaTime();

        {
            LIMBO_PROFILE_SCOPE("Input");
            // Update input state before polling events
            Input::update();
            InputManager::update();
            m_window->pollEvents();
        }

        // Handle escape key to close
        if (Input::isKeyPressed(Key::Escape)) {
            m_window->setShouldClose(true);
        }

        // Fixed timestep updates (physics, deterministic logic)
        {
            LIMBO_PROFILE_SCOPE("FixedUpdate");
            while (Time::shouldFixedUpdate()) {
                m_systems.fixedUpdate(m_world, fixedDeltaTime);
                onFixedUpdate(fixedDeltaTime);
            }
        }

        // Variable timestep update (animations, AI, etc.)
        {
            LIMBO_PROFILE_SCOPE("Update");
            m_systems.update(m_world, deltaTime);
            onUpdate(deltaTime);
        }

        // Render with interpolation alpha for smooth visuals
        // Process main thread queue (GPU uploads, etc. from worker threads)
        {
            LIMBO_PROFILE_SCOPE("MainThreadQueue");
            MainThreadQueue::processAll();
        }

        // Process async asset loading (GPU uploads)
        {
            LIMBO_PROFILE_SCOPE("AssetLoader");
            AssetLoader::processMainThreadWork();
        }

        // Render with interpolation alpha for smooth visuals
        {
            LIMBO_PROFILE_SCOPE("Render");
            f32 const alpha = Time::getInterpolationAlpha();
            onRender(alpha);
        }

        {
            LIMBO_PROFILE_SCOPE("SwapBuffers");
            m_window->swapBuffers();
        }

        profiler::Profiler::endFrame();
    }

    spdlog::info("Exiting main loop");
}

void Application::shutdown() {
    spdlog::info("Shutting down application");

    // Shutdown systems first
    m_systems.shutdown(m_world);

    onShutdown();

    // Clear all entities
    m_world.clear();

    // Shutdown async asset loader
    AssetLoader::shutdown();

    // Shutdown thread pool (wait for all jobs to complete)
    ThreadPool::shutdown();

    // Process any remaining main thread tasks
    MainThreadQueue::processAll();

    // Shutdown frame allocator
    frame::shutdown();

    // Shutdown profiler
    profiler::Profiler::shutdown();

    // Shutdown input manager
    InputManager::shutdown();

    m_window.reset();
    platform::shutdown();

    spdlog::info("Application shutdown complete");
}

void Application::requestExit() {
    m_running = false;
}

Application& Application::get() {
    return *s_instance;
}

}  // namespace limbo
