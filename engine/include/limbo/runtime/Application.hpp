#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/platform/Platform.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/System.hpp"

namespace limbo {

struct ApplicationConfig {
    WindowConfig window;
    String appName = "Limbo Application";
};

class LIMBO_API Application {
public:
    Application() = default;
    virtual ~Application();

    LIMBO_NON_COPYABLE_NON_MOVABLE(Application);

    [[nodiscard]] Result<void> init(const ApplicationConfig& config);
    void run();
    void shutdown();

    void requestExit();

    [[nodiscard]] Window& getWindow() { return *m_window; }
    [[nodiscard]] const Window& getWindow() const { return *m_window; }

    [[nodiscard]] World& getWorld() { return m_world; }
    [[nodiscard]] const World& getWorld() const { return m_world; }

    [[nodiscard]] SystemManager& getSystems() { return m_systems; }
    [[nodiscard]] const SystemManager& getSystems() const { return m_systems; }

    [[nodiscard]] static Application& get();

protected:
    virtual void onInit() {}
    virtual void onUpdate([[maybe_unused]] f32 deltaTime) {}
    virtual void onRender() {}
    virtual void onShutdown() {}

private:
    Unique<Window> m_window;
    World m_world;
    SystemManager m_systems;
    bool m_running = false;
    f64 m_lastFrameTime = 0.0;

    static Application* s_instance;
};

}  // namespace limbo
