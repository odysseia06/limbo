#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <chrono>
#include <thread>
#include <vector>

namespace limbo::profiler {

/**
 * ProfilerSample - A single timing measurement
 */
struct ProfilerSample {
    const char* name = nullptr;    // Static string (no allocation)
    u64 startTime = 0;             // Nanoseconds since epoch
    u64 endTime = 0;               // Nanoseconds since epoch
    u32 depth = 0;                 // Nesting level (0 = top-level)
    u32 parentIndex = 0;           // Index of parent sample (0 = none)
    std::thread::id threadId;      // Thread that recorded this sample

    [[nodiscard]] f64 getDurationMs() const {
        return static_cast<f64>(endTime - startTime) / 1'000'000.0;
    }

    [[nodiscard]] f64 getDurationUs() const {
        return static_cast<f64>(endTime - startTime) / 1'000.0;
    }
};

/**
 * FrameData - All profiler data for a single frame
 */
struct FrameData {
    std::vector<ProfilerSample> samples;
    u64 frameNumber = 0;
    u64 frameStartTime = 0;
    u64 frameEndTime = 0;

    [[nodiscard]] f64 getFrameDurationMs() const {
        return static_cast<f64>(frameEndTime - frameStartTime) / 1'000'000.0;
    }

    void clear() {
        samples.clear();
        frameNumber = 0;
        frameStartTime = 0;
        frameEndTime = 0;
    }
};

/**
 * Profiler - Hierarchical CPU profiling system
 *
 * Usage:
 *   profiler::init();
 *
 *   // In main loop:
 *   profiler::beginFrame();
 *   {
 *       LIMBO_PROFILE_SCOPE("Update");
 *       // ... work ...
 *   }
 *   profiler::endFrame();
 *
 *   // Capture for analysis:
 *   profiler::captureFrame();
 *   const auto* frame = profiler::getCapturedFrame();
 */
class LIMBO_API Profiler {
public:
    /**
     * Initialize the profiler system
     * @param maxSamplesPerFrame Maximum samples per frame (default 4096)
     * @param historyFrames Number of frames to keep in history (default 120)
     */
    static void init(u32 maxSamplesPerFrame = 4096, u32 historyFrames = 120);

    /**
     * Shutdown the profiler system
     */
    static void shutdown();

    /**
     * Begin a new frame
     * Call at the start of each frame before any profiled code
     */
    static void beginFrame();

    /**
     * End the current frame
     * Call at the end of each frame after all profiled code
     */
    static void endFrame();

    /**
     * Begin a profiler sample
     * @param name Static string name (must outlive the sample)
     */
    static void beginSample(const char* name);

    /**
     * End the current profiler sample
     */
    static void endSample();

    /**
     * Capture the current frame for detailed analysis
     * The captured frame persists until the next capture
     */
    static void captureFrame();

    /**
     * Get the most recently captured frame
     * @return Pointer to captured frame data, or nullptr if none captured
     */
    [[nodiscard]] static const FrameData* getCapturedFrame();

    /**
     * Get the most recent completed frame
     * @return Pointer to last frame data
     */
    [[nodiscard]] static const FrameData* getLastFrame();

    /**
     * Get frame history for graphs/analysis
     * @return Reference to frame history ring buffer
     */
    [[nodiscard]] static const std::vector<FrameData>& getHistory();

    /**
     * Get current frame number
     */
    [[nodiscard]] static u64 getFrameNumber();

    /**
     * Export captured frame to CSV
     * @param filepath Path to output file
     * @return True if export succeeded
     */
    static bool exportToCSV(const char* filepath);

    /**
     * Enable or disable profiling at runtime
     * When disabled, begin/endSample become no-ops
     */
    static void setEnabled(bool enabled);

    /**
     * Check if profiling is enabled
     */
    [[nodiscard]] static bool isEnabled();

    /**
     * Check if profiler has been initialized
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * Get high-resolution timestamp in nanoseconds
     */
    [[nodiscard]] static u64 getTimestamp();
};

/**
 * ScopedSample - RAII helper for profiler samples
 */
class ScopedSample {
public:
    explicit ScopedSample(const char* name) { Profiler::beginSample(name); }
    ~ScopedSample() { Profiler::endSample(); }

    // Non-copyable, non-movable
    ScopedSample(const ScopedSample&) = delete;
    ScopedSample& operator=(const ScopedSample&) = delete;
    ScopedSample(ScopedSample&&) = delete;
    ScopedSample& operator=(ScopedSample&&) = delete;
};

}  // namespace limbo::profiler

// ============================================================================
// Profiler Macros
// ============================================================================
// These compile away in release builds when LIMBO_PROFILING_ENABLED is not defined

#if defined(LIMBO_DEBUG) || defined(LIMBO_PROFILING_ENABLED)
    #define LIMBO_PROFILING_ENABLED 1

    // Profile a named scope
    #define LIMBO_PROFILE_SCOPE(name)                                                              \
        ::limbo::profiler::ScopedSample LIMBO_CONCAT(_profiler_sample_, __LINE__)(name)

    // Profile a function (uses function name)
    #define LIMBO_PROFILE_FUNCTION() LIMBO_PROFILE_SCOPE(__FUNCTION__)

    // Profile with custom category prefix
    #define LIMBO_PROFILE_CATEGORY(category, name)                                                 \
        ::limbo::profiler::ScopedSample LIMBO_CONCAT(_profiler_sample_, __LINE__)("[" category     \
                                                                                  "] " name)
#else
    #define LIMBO_PROFILE_SCOPE(name) ((void)0)
    #define LIMBO_PROFILE_FUNCTION() ((void)0)
    #define LIMBO_PROFILE_CATEGORY(category, name) ((void)0)
#endif
