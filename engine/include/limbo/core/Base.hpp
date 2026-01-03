#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define LIMBO_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define LIMBO_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#define LIMBO_PLATFORM_MACOS 1
#else
#error "Unsupported platform"
#endif

// Compiler detection
#if defined(_MSC_VER)
#define LIMBO_COMPILER_MSVC 1
#elif defined(__clang__)
#define LIMBO_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define LIMBO_COMPILER_GCC 1
#else
#define LIMBO_COMPILER_UNKNOWN 1
#endif

// DLL export/import macros
#if defined(LIMBO_SHARED_LIB)
#if defined(LIMBO_PLATFORM_WINDOWS)
#if defined(LIMBO_EXPORT)
#define LIMBO_API __declspec(dllexport)
#else
#define LIMBO_API __declspec(dllimport)
#endif
#else
#define LIMBO_API __attribute__((visibility("default")))
#endif
#else
#define LIMBO_API
#endif

// Debug break
#if defined(LIMBO_DEBUG)
#if defined(LIMBO_PLATFORM_WINDOWS)
#define LIMBO_DEBUG_BREAK() __debugbreak()
#elif defined(LIMBO_COMPILER_CLANG) || defined(LIMBO_COMPILER_GCC)
#define LIMBO_DEBUG_BREAK() __builtin_trap()
#else
#define LIMBO_DEBUG_BREAK()
#endif
#else
#define LIMBO_DEBUG_BREAK()
#endif

// Utility macros
#define LIMBO_UNUSED(x) (void)(x)
#define LIMBO_STRINGIFY(x) #x
#define LIMBO_CONCAT(a, b) a##b

// Disable copy and move
#define LIMBO_NON_COPYABLE(ClassName)                                                              \
    ClassName(const ClassName&) = delete;                                                          \
    ClassName& operator=(const ClassName&) = delete

#define LIMBO_NON_MOVABLE(ClassName)                                                               \
    ClassName(ClassName&&) = delete;                                                               \
    ClassName& operator=(ClassName&&) = delete

#define LIMBO_NON_COPYABLE_NON_MOVABLE(ClassName)                                                  \
    LIMBO_NON_COPYABLE(ClassName);                                                                 \
    LIMBO_NON_MOVABLE(ClassName)

// Version info
#define LIMBO_VERSION_MAJOR 0
#define LIMBO_VERSION_MINOR 1
#define LIMBO_VERSION_PATCH 0

// OpenGL calling convention
#if defined(LIMBO_PLATFORM_WINDOWS)
#define LIMBO_GL_CALLBACK __stdcall
#else
#define LIMBO_GL_CALLBACK
#endif
