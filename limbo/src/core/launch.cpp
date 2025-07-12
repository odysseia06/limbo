#include "limbo/core/launch.hpp"
#include "limbo/core/platform.hpp"
#include "limbo/core/log.hpp"

namespace limbo {

    int Launch(const LaunchConfig& cfg, Application* (*factory)())
    {
        Platform plat;
        if (!plat.init(cfg.width, cfg.height, "Limbo Sandbox"))
            return -1;

        auto* app = factory();
        if (!app) {
            limbo::log::error("Factory returned nullptr");
            return -2;
        }

        double prev = plat.time_seconds();
        while (!plat.should_close()) {
            double now = plat.time_seconds();
            double dt = now - prev;  prev = now;

            plat.poll_events([&](const Event& e) { app->on_event(e); });
            app->on_update(dt);
            plat.swap_buffers(cfg.vsync);
        }

        delete app;
        plat.shutdown();
        return 0;
    }

} // namespace limbo
