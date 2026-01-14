#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <array>

namespace limbo {

/**
 * GPUTimer - Measures GPU execution time using OpenGL timer queries
 *
 * Uses GL_TIME_ELAPSED queries to measure actual GPU time spent on rendering.
 * Implements double-buffering to avoid stalls when reading query results.
 *
 * Usage:
 *   GPUTimer timer;
 *   timer.init();
 *
 *   // Each frame:
 *   timer.beginFrame();
 *   timer.begin("RenderScene");
 *   // ... render scene ...
 *   timer.end();
 *   timer.begin("PostProcess");
 *   // ... post process ...
 *   timer.end();
 *   timer.endFrame();
 *
 *   // Get results (from previous frame due to double-buffering)
 *   f64 sceneTime = timer.getTimeMs("RenderScene");
 *   f64 totalTime = timer.getTotalTimeMs();
 */
class LIMBO_API GPUTimer {
public:
    static constexpr u32 MaxTimers = 16;
    static constexpr u32 BufferCount = 2;  // Double-buffering

    GPUTimer() = default;
    ~GPUTimer();

    // Non-copyable
    GPUTimer(const GPUTimer&) = delete;
    GPUTimer& operator=(const GPUTimer&) = delete;

    /**
     * Initialize GPU timer queries
     * Must be called after OpenGL context is created
     */
    void init();

    /**
     * Shutdown and release OpenGL resources
     */
    void shutdown();

    /**
     * Check if timer is initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * Begin a new frame of timing
     * Call at the start of each frame
     */
    void beginFrame();

    /**
     * End the current frame
     * Call at the end of each frame after all timed sections
     */
    void endFrame();

    /**
     * Begin timing a named section
     * @param name Section name (max 31 characters)
     */
    void begin(const char* name);

    /**
     * End the current timing section
     */
    void end();

    /**
     * Get the GPU time for a named section in milliseconds
     * Returns the result from the previous frame (due to double-buffering)
     * @param name Section name
     * @return Time in milliseconds, or 0 if not found
     */
    [[nodiscard]] f64 getTimeMs(const char* name) const;

    /**
     * Get the total GPU time for the frame in milliseconds
     */
    [[nodiscard]] f64 getTotalTimeMs() const { return m_totalTimeMs; }

    /**
     * Get the number of active timers
     */
    [[nodiscard]] u32 getTimerCount() const { return m_timerCount; }

    /**
     * Get timer name by index
     */
    [[nodiscard]] const char* getTimerName(u32 index) const;

    /**
     * Get timer time by index in milliseconds
     */
    [[nodiscard]] f64 getTimerTimeMs(u32 index) const;

    /**
     * Check if GPU timer queries are supported
     */
    [[nodiscard]] static bool isSupported();

private:
    struct TimerData {
        char name[32] = {};
        u32 queryIds[BufferCount] = {};
        f64 timeMs = 0.0;
        bool active = false;
    };

    void collectResults();

    std::array<TimerData, MaxTimers> m_timers;
    u32 m_timerCount = 0;
    u32 m_currentBuffer = 0;
    u32 m_activeTimer = MaxTimers;  // Invalid index = no active timer
    f64 m_totalTimeMs = 0.0;
    bool m_initialized = false;
    bool m_frameActive = false;
};

/**
 * ScopedGPUTimer - RAII helper for GPU timing sections
 *
 * Usage:
 *   {
 *       ScopedGPUTimer timer(gpuTimer, "RenderScene");
 *       // ... render scene ...
 *   } // automatically calls end()
 */
class LIMBO_API ScopedGPUTimer {
public:
    ScopedGPUTimer(GPUTimer& timer, const char* name) : m_timer(timer) { m_timer.begin(name); }

    ~ScopedGPUTimer() { m_timer.end(); }

    // Non-copyable, non-movable
    ScopedGPUTimer(const ScopedGPUTimer&) = delete;
    ScopedGPUTimer& operator=(const ScopedGPUTimer&) = delete;
    ScopedGPUTimer(ScopedGPUTimer&&) = delete;
    ScopedGPUTimer& operator=(ScopedGPUTimer&&) = delete;

private:
    GPUTimer& m_timer;
};

}  // namespace limbo
