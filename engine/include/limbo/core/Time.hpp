#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <array>

namespace limbo {

/**
 * Time configuration for the game loop
 */
struct TimeConfig {
    // Fixed timestep for physics and deterministic updates (default: 60 Hz)
    f32 fixedDeltaTime = 1.0f / 60.0f;

    // Maximum delta time to prevent spiral of death (default: 250ms)
    f32 maxDeltaTime = 0.25f;

    // Number of frames to average for delta smoothing (default: 11)
    u32 smoothingFrames = 11;

    // Enable delta time smoothing
    bool enableSmoothing = true;

    // Maximum fixed updates per frame to prevent lockup
    u32 maxFixedUpdatesPerFrame = 8;
};

/**
 * Time - Manages game time, fixed timestep, and frame timing
 *
 * Provides:
 * - Variable delta time for rendering and animations
 * - Fixed delta time for physics and deterministic logic
 * - Frame time smoothing to reduce jitter
 * - Time scaling for slow-motion effects
 */
class LIMBO_API Time {
public:
    /**
     * Initialize the time system with configuration
     */
    static void init(const TimeConfig& config = {});

    /**
     * Reset the time system (call when starting a new scene, etc.)
     */
    static void reset();

    /**
     * Begin a new frame - call at the start of the game loop
     * @return The raw delta time since last frame
     */
    static f32 beginFrame();

    /**
     * Check if a fixed update should be performed
     * Call this in a loop until it returns false
     * @return true if fixedUpdate should be called
     */
    static bool shouldFixedUpdate();

    /**
     * Get the interpolation alpha for rendering between physics states
     * Value between 0 and 1 representing position between last and current physics state
     */
    [[nodiscard]] static f32 getInterpolationAlpha();

    // ========================================================================
    // Time Queries
    // ========================================================================

    /**
     * Get the (potentially smoothed) delta time for this frame
     * Use for animations, movement, etc.
     */
    [[nodiscard]] static f32 getDeltaTime();

    /**
     * Get the raw unsmoothed delta time
     */
    [[nodiscard]] static f32 getRawDeltaTime();

    /**
     * Get the fixed delta time for physics updates
     */
    [[nodiscard]] static f32 getFixedDeltaTime();

    /**
     * Get time since the application started (in seconds)
     */
    [[nodiscard]] static f64 getTimeSinceStart();

    /**
     * Get the current frame number
     */
    [[nodiscard]] static u64 getFrameCount();

    /**
     * Get the current frames per second (averaged)
     */
    [[nodiscard]] static f32 getFPS();

    // ========================================================================
    // Time Scale
    // ========================================================================

    /**
     * Set the time scale (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
     * Affects deltaTime but not fixedDeltaTime
     */
    static void setTimeScale(f32 scale);

    /**
     * Get the current time scale
     */
    [[nodiscard]] static f32 getTimeScale();

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Get the current time configuration
     */
    [[nodiscard]] static const TimeConfig& getConfig();

    /**
     * Update the time configuration
     */
    static void setConfig(const TimeConfig& config);

private:
    Time() = default;
};

}  // namespace limbo
