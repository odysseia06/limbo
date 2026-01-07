#include "limbo/runtime/Application.hpp"
#include "limbo/core/Time.hpp"
#include "limbo/platform/Input.hpp"
#include "limbo/input/InputManager.hpp"

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
        // Begin frame timing
        Time::beginFrame();
        f32 const deltaTime = Time::getDeltaTime();
        f32 const fixedDeltaTime = Time::getFixedDeltaTime();

        // Update input state before polling events
        Input::update();
        InputManager::update();

        m_window->pollEvents();

        // Handle escape key to close
        if (Input::isKeyPressed(Key::Escape)) {
            m_window->setShouldClose(true);
        }

        // Fixed timestep updates (physics, deterministic logic)
        while (Time::shouldFixedUpdate()) {
            m_systems.fixedUpdate(m_world, fixedDeltaTime);
            onFixedUpdate(fixedDeltaTime);
        }

        // Variable timestep update (animations, AI, etc.)
        m_systems.update(m_world, deltaTime);
        onUpdate(deltaTime);

        // Render with interpolation alpha for smooth visuals
        f32 const alpha = Time::getInterpolationAlpha();
        onRender(alpha);

        m_window->swapBuffers();
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
