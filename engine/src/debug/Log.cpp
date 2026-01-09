#include "limbo/debug/Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>

#include <memory>
#include <mutex>
#include <vector>

namespace limbo::log {

namespace {

// Logger instances
std::shared_ptr<spdlog::logger> s_coreLogger;
std::shared_ptr<spdlog::logger> s_renderLogger;
std::shared_ptr<spdlog::logger> s_physicsLogger;
std::shared_ptr<spdlog::logger> s_audioLogger;
std::shared_ptr<spdlog::logger> s_scriptLogger;
std::shared_ptr<spdlog::logger> s_editorLogger;
std::shared_ptr<spdlog::logger> s_assetLogger;
std::shared_ptr<spdlog::logger> s_inputLogger;
std::shared_ptr<spdlog::logger> s_ecsLogger;

// Callback sink for routing logs to the debug console
std::vector<LogCallback> s_logCallbacks;
std::mutex s_callbackMutex;

// Custom sink that forwards to callbacks
template <typename Mutex>
class CallbackSink : public spdlog::sinks::base_sink<Mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        if (s_logCallbacks.empty()) {
            return;
        }

        LogEntry entry;
        entry.message = std::string(msg.payload.data(), msg.payload.size());
        entry.category = std::string(msg.logger_name.data(), msg.logger_name.size());
        entry.level = msg.level;

        for (const auto& callback : s_logCallbacks) {
            if (callback) {
                callback(entry);
            }
        }
    }

    void flush_() override {}
};

using CallbackSinkMt = CallbackSink<std::mutex>;

std::shared_ptr<CallbackSinkMt> s_callbackSink;

std::shared_ptr<spdlog::logger> createLogger(const std::string& name,
                                             std::vector<spdlog::sink_ptr>& sinks) {
    auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());

#ifdef LIMBO_DEBUG
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%n] %v");
#else
    logger->set_level(spdlog::level::info);
    logger->set_pattern("[%H:%M:%S] [%^%l%$] [%n] %v");
#endif

    spdlog::register_logger(logger);
    return logger;
}

}  // namespace

void init() {
    // Create shared sinks
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink for terminal output
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#ifdef LIMBO_DEBUG
    consoleSink->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%n] %v");
#else
    consoleSink->set_pattern("[%H:%M:%S] [%^%l%$] [%n] %v");
#endif
    sinks.push_back(consoleSink);

    // Callback sink for debug console UI
    s_callbackSink = std::make_shared<CallbackSinkMt>();
    sinks.push_back(s_callbackSink);

    // Create category loggers
    s_coreLogger = createLogger("CORE", sinks);
    s_renderLogger = createLogger("RENDER", sinks);
    s_physicsLogger = createLogger("PHYSICS", sinks);
    s_audioLogger = createLogger("AUDIO", sinks);
    s_scriptLogger = createLogger("SCRIPT", sinks);
    s_editorLogger = createLogger("EDITOR", sinks);
    s_assetLogger = createLogger("ASSET", sinks);
    s_inputLogger = createLogger("INPUT", sinks);
    s_ecsLogger = createLogger("ECS", sinks);

    // Set core logger as default
    spdlog::set_default_logger(s_coreLogger);

    SPDLOG_LOGGER_INFO(s_coreLogger, "Logging system initialized");
}

void addLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(s_callbackMutex);
    s_logCallbacks.push_back(std::move(callback));
}

void clearLogCallbacks() {
    std::lock_guard<std::mutex> lock(s_callbackMutex);
    s_logCallbacks.clear();
}

void shutdown() {
    SPDLOG_LOGGER_INFO(s_coreLogger, "Logging system shutdown");

    s_coreLogger.reset();
    s_renderLogger.reset();
    s_physicsLogger.reset();
    s_audioLogger.reset();
    s_scriptLogger.reset();
    s_editorLogger.reset();
    s_assetLogger.reset();
    s_inputLogger.reset();
    s_ecsLogger.reset();

    spdlog::shutdown();
}

spdlog::logger& core() {
    return *s_coreLogger;
}

spdlog::logger& render() {
    return *s_renderLogger;
}

spdlog::logger& physics() {
    return *s_physicsLogger;
}

spdlog::logger& audio() {
    return *s_audioLogger;
}

spdlog::logger& script() {
    return *s_scriptLogger;
}

spdlog::logger& editor() {
    return *s_editorLogger;
}

spdlog::logger& asset() {
    return *s_assetLogger;
}

spdlog::logger& input() {
    return *s_inputLogger;
}

spdlog::logger& ecs() {
    return *s_ecsLogger;
}

}  // namespace limbo::log
