#pragma once

#include "limbo/core/Base.hpp"

#include <spdlog/spdlog.h>

// LIMBO_ASSERT - Hard assertion that breaks in debug, stripped in release
// Use for conditions that should NEVER be false (programmer errors)
#ifdef LIMBO_DEBUG
#define LIMBO_ASSERT(condition, ...)                                                               \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            SPDLOG_CRITICAL("Assertion failed: {}", #condition);                                   \
            if constexpr (sizeof(__VA_ARGS__) > 1) {                                               \
                SPDLOG_CRITICAL(__VA_ARGS__);                                                      \
            }                                                                                      \
            LIMBO_DEBUG_BREAK();                                                                   \
        }                                                                                          \
    } while (false)
#else
#define LIMBO_ASSERT(condition, ...) ((void)0)
#endif

// LIMBO_VERIFY - Like ASSERT but expression is always evaluated (even in release)
// Use when the condition has side effects that must execute
#ifdef LIMBO_DEBUG
#define LIMBO_VERIFY(condition, ...)                                                               \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            SPDLOG_CRITICAL("Verification failed: {}", #condition);                                \
            if constexpr (sizeof(__VA_ARGS__) > 1) {                                               \
                SPDLOG_CRITICAL(__VA_ARGS__);                                                      \
            }                                                                                      \
            LIMBO_DEBUG_BREAK();                                                                   \
        }                                                                                          \
    } while (false)
#else
#define LIMBO_VERIFY(condition, ...) ((void)(condition))
#endif

// LIMBO_ENSURE - Soft assertion that logs and continues (never stripped)
// Use for recoverable errors or unexpected but handleable conditions
#define LIMBO_ENSURE(condition, ...)                                                               \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            SPDLOG_ERROR("Ensure failed: {}", #condition);                                         \
            if constexpr (sizeof(__VA_ARGS__) > 1) {                                               \
                SPDLOG_ERROR(__VA_ARGS__);                                                         \
            }                                                                                      \
        }                                                                                          \
    } while (false)

// LIMBO_ENSURE_MSG - Returns false if condition fails (for early returns)
// Usage: if (!LIMBO_ENSURE_RET(ptr != nullptr, "Null pointer")) return;
#define LIMBO_ENSURE_RET(condition, ...)                                                           \
    [&]() -> bool {                                                                                \
        if (!(condition)) {                                                                        \
            SPDLOG_ERROR("Ensure failed: {}", #condition);                                         \
            if constexpr (sizeof(__VA_ARGS__) > 1) {                                               \
                SPDLOG_ERROR(__VA_ARGS__);                                                         \
            }                                                                                      \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }()

// LIMBO_UNREACHABLE - Marks code paths that should never be reached
#ifdef LIMBO_DEBUG
#define LIMBO_UNREACHABLE()                                                                        \
    do {                                                                                           \
        SPDLOG_CRITICAL("Unreachable code reached at {}:{}", __FILE__, __LINE__);                  \
        LIMBO_DEBUG_BREAK();                                                                       \
    } while (false)
#else
#if defined(LIMBO_COMPILER_MSVC)
#define LIMBO_UNREACHABLE() __assume(0)
#elif defined(LIMBO_COMPILER_GCC) || defined(LIMBO_COMPILER_CLANG)
#define LIMBO_UNREACHABLE() __builtin_unreachable()
#else
#define LIMBO_UNREACHABLE() ((void)0)
#endif
#endif

// LIMBO_NOT_IMPLEMENTED - Marks code that is not yet implemented
#define LIMBO_NOT_IMPLEMENTED()                                                                    \
    do {                                                                                           \
        SPDLOG_WARN("Not implemented: {} at {}:{}", __FUNCTION__, __FILE__, __LINE__);             \
        LIMBO_DEBUG_BREAK();                                                                       \
    } while (false)
