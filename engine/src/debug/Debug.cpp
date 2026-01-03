#include "limbo/debug/Debug.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace limbo::debug {

void init() {
    // Configure spdlog
    auto console = spdlog::stdout_color_mt("limbo");
    spdlog::set_default_logger(console);

#ifdef LIMBO_DEBUG
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
#else
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
#endif

    spdlog::info("Debug system initialized");
}

void shutdown() {
    spdlog::info("Debug system shutdown");
    spdlog::shutdown();
}

} // namespace limbo::debug
