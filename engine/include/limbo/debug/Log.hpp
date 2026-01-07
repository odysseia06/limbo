#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

#include <functional>

namespace limbo::log {

// Log entry for the debug console
struct LogEntry {
    String message;
    String category;
    spdlog::level::level_enum level;
};

// Callback type for log listeners
using LogCallback = std::function<void(const LogEntry&)>;

// Initialize all log categories (called by debug::init())
LIMBO_API void init();

// Shutdown all loggers (called by debug::shutdown())
LIMBO_API void shutdown();

// Register a callback to receive log messages (for debug console)
LIMBO_API void addLogCallback(LogCallback callback);

// Clear all log callbacks
LIMBO_API void clearLogCallbacks();

// Get loggers for each subsystem
LIMBO_API spdlog::logger& core();
LIMBO_API spdlog::logger& render();
LIMBO_API spdlog::logger& physics();
LIMBO_API spdlog::logger& audio();
LIMBO_API spdlog::logger& script();
LIMBO_API spdlog::logger& editor();
LIMBO_API spdlog::logger& asset();
LIMBO_API spdlog::logger& input();
LIMBO_API spdlog::logger& ecs();

}  // namespace limbo::log

// =============================================================================
// Core logging macros
// =============================================================================
#define LIMBO_LOG_CORE_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::core(), __VA_ARGS__)
#define LIMBO_LOG_CORE_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::core(), __VA_ARGS__)
#define LIMBO_LOG_CORE_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::core(), __VA_ARGS__)
#define LIMBO_LOG_CORE_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::core(), __VA_ARGS__)
#define LIMBO_LOG_CORE_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::core(), __VA_ARGS__)
#define LIMBO_LOG_CORE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::core(), __VA_ARGS__)

// =============================================================================
// Render logging macros
// =============================================================================
#define LIMBO_LOG_RENDER_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::render(), __VA_ARGS__)
#define LIMBO_LOG_RENDER_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::render(), __VA_ARGS__)
#define LIMBO_LOG_RENDER_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::render(), __VA_ARGS__)
#define LIMBO_LOG_RENDER_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::render(), __VA_ARGS__)
#define LIMBO_LOG_RENDER_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::render(), __VA_ARGS__)
#define LIMBO_LOG_RENDER_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::render(), __VA_ARGS__)

// =============================================================================
// Physics logging macros
// =============================================================================
#define LIMBO_LOG_PHYSICS_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::physics(), __VA_ARGS__)
#define LIMBO_LOG_PHYSICS_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::physics(), __VA_ARGS__)
#define LIMBO_LOG_PHYSICS_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::physics(), __VA_ARGS__)
#define LIMBO_LOG_PHYSICS_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::physics(), __VA_ARGS__)
#define LIMBO_LOG_PHYSICS_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::physics(), __VA_ARGS__)
#define LIMBO_LOG_PHYSICS_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::physics(), __VA_ARGS__)

// =============================================================================
// Audio logging macros
// =============================================================================
#define LIMBO_LOG_AUDIO_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::audio(), __VA_ARGS__)
#define LIMBO_LOG_AUDIO_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::audio(), __VA_ARGS__)
#define LIMBO_LOG_AUDIO_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::audio(), __VA_ARGS__)
#define LIMBO_LOG_AUDIO_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::audio(), __VA_ARGS__)
#define LIMBO_LOG_AUDIO_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::audio(), __VA_ARGS__)
#define LIMBO_LOG_AUDIO_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::audio(), __VA_ARGS__)

// =============================================================================
// Script logging macros
// =============================================================================
#define LIMBO_LOG_SCRIPT_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::script(), __VA_ARGS__)
#define LIMBO_LOG_SCRIPT_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::script(), __VA_ARGS__)
#define LIMBO_LOG_SCRIPT_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::script(), __VA_ARGS__)
#define LIMBO_LOG_SCRIPT_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::script(), __VA_ARGS__)
#define LIMBO_LOG_SCRIPT_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::script(), __VA_ARGS__)
#define LIMBO_LOG_SCRIPT_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::script(), __VA_ARGS__)

// =============================================================================
// Editor logging macros
// =============================================================================
#define LIMBO_LOG_EDITOR_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::editor(), __VA_ARGS__)
#define LIMBO_LOG_EDITOR_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::editor(), __VA_ARGS__)
#define LIMBO_LOG_EDITOR_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::editor(), __VA_ARGS__)
#define LIMBO_LOG_EDITOR_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::editor(), __VA_ARGS__)
#define LIMBO_LOG_EDITOR_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::editor(), __VA_ARGS__)
#define LIMBO_LOG_EDITOR_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::editor(), __VA_ARGS__)

// =============================================================================
// Asset logging macros
// =============================================================================
#define LIMBO_LOG_ASSET_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::asset(), __VA_ARGS__)
#define LIMBO_LOG_ASSET_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::asset(), __VA_ARGS__)
#define LIMBO_LOG_ASSET_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::asset(), __VA_ARGS__)
#define LIMBO_LOG_ASSET_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::asset(), __VA_ARGS__)
#define LIMBO_LOG_ASSET_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::asset(), __VA_ARGS__)
#define LIMBO_LOG_ASSET_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::asset(), __VA_ARGS__)

// =============================================================================
// Input logging macros
// =============================================================================
#define LIMBO_LOG_INPUT_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::input(), __VA_ARGS__)
#define LIMBO_LOG_INPUT_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::input(), __VA_ARGS__)
#define LIMBO_LOG_INPUT_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::input(), __VA_ARGS__)
#define LIMBO_LOG_INPUT_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::input(), __VA_ARGS__)
#define LIMBO_LOG_INPUT_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::input(), __VA_ARGS__)
#define LIMBO_LOG_INPUT_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::input(), __VA_ARGS__)

// =============================================================================
// ECS logging macros
// =============================================================================
#define LIMBO_LOG_ECS_TRACE(...)    SPDLOG_LOGGER_TRACE(&::limbo::log::ecs(), __VA_ARGS__)
#define LIMBO_LOG_ECS_DEBUG(...)    SPDLOG_LOGGER_DEBUG(&::limbo::log::ecs(), __VA_ARGS__)
#define LIMBO_LOG_ECS_INFO(...)     SPDLOG_LOGGER_INFO(&::limbo::log::ecs(), __VA_ARGS__)
#define LIMBO_LOG_ECS_WARN(...)     SPDLOG_LOGGER_WARN(&::limbo::log::ecs(), __VA_ARGS__)
#define LIMBO_LOG_ECS_ERROR(...)    SPDLOG_LOGGER_ERROR(&::limbo::log::ecs(), __VA_ARGS__)
#define LIMBO_LOG_ECS_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(&::limbo::log::ecs(), __VA_ARGS__)
