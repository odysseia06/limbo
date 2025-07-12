#pragma once
#include <limbo/app/application.hpp>

namespace limbo {

    struct LaunchConfig {
        int  width = 1280;
        int  height = 720;
        bool vsync = true;
    };

    /// Engine entry point (game owns main()).
    /// @param createApp factory returning new Application*
    int Launch(const LaunchConfig& cfg,
        Application* (*createApp)());

} // namespace limbo

