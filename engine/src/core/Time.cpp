#include "limbo/core/Time.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>

namespace limbo {

namespace {

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<f64>;

struct TimeState {
    TimeConfig config;

    // Frame timing
    TimePoint startTime;
    TimePoint lastFrameTime;
    f32 rawDeltaTime = 0.0f;
    f32 smoothedDeltaTime = 0.0f;
    u64 frameCount = 0;

    // Delta smoothing buffer (circular)
    std::array<f32, 32> deltaSamples{};
    u32 sampleIndex = 0;
    u32 sampleCount = 0;

    // Fixed timestep accumulator
    f32 accumulator = 0.0f;
    u32 fixedUpdatesThisFrame = 0;

    // Time scale
    f32 timeScale = 1.0f;

    // FPS calculation
    f32 fpsAccumulator = 0.0f;
    u32 fpsFrameCount = 0;
    f32 currentFPS = 0.0f;

    bool initialized = false;
};

TimeState& getState() {
    static TimeState state;
    return state;
}

f32 calculateSmoothedDelta(TimeState& state) {
    if (!state.config.enableSmoothing || state.sampleCount == 0) {
        return state.rawDeltaTime;
    }

    // Calculate how many samples to use
    u32 samplesToUse = std::min(state.sampleCount, state.config.smoothingFrames);
    samplesToUse = std::min(samplesToUse, static_cast<u32>(state.deltaSamples.size()));

    if (samplesToUse == 0) {
        return state.rawDeltaTime;
    }

    // Gather the most recent samples
    std::array<f32, 32> sortedSamples{};
    for (u32 i = 0; i < samplesToUse; ++i) {
        auto idx = (state.sampleIndex + static_cast<u32>(state.deltaSamples.size()) - 1 - i) %
                   static_cast<u32>(state.deltaSamples.size());
        sortedSamples[i] = state.deltaSamples[idx];
    }

    // Sort and take median to filter outliers
    std::sort(sortedSamples.begin(), sortedSamples.begin() + samplesToUse);

    // Use trimmed mean: discard lowest and highest if we have enough samples
    if (samplesToUse >= 5) {
        u32 trimCount = samplesToUse / 4;  // Trim 25% from each end
        f32 sum = 0.0f;
        u32 count = samplesToUse - 2 * trimCount;
        for (u32 i = trimCount; i < samplesToUse - trimCount; ++i) {
            sum += sortedSamples[i];
        }
        return sum / static_cast<f32>(count);
    }

    // For few samples, just use average
    f32 sum = std::accumulate(sortedSamples.begin(), sortedSamples.begin() + samplesToUse, 0.0f);
    return sum / static_cast<f32>(samplesToUse);
}

}  // namespace

void Time::init(const TimeConfig& config) {
    auto& state = getState();
    state.config = config;
    state.startTime = Clock::now();
    state.lastFrameTime = state.startTime;
    state.rawDeltaTime = config.fixedDeltaTime;
    state.smoothedDeltaTime = config.fixedDeltaTime;
    state.frameCount = 0;
    state.sampleIndex = 0;
    state.sampleCount = 0;
    state.accumulator = 0.0f;
    state.fixedUpdatesThisFrame = 0;
    state.timeScale = 1.0f;
    state.fpsAccumulator = 0.0f;
    state.fpsFrameCount = 0;
    state.currentFPS = 0.0f;
    state.deltaSamples.fill(0.0f);
    state.initialized = true;
}

void Time::reset() {
    auto& state = getState();
    state.lastFrameTime = Clock::now();
    state.accumulator = 0.0f;
    state.fixedUpdatesThisFrame = 0;
    state.sampleIndex = 0;
    state.sampleCount = 0;
    state.deltaSamples.fill(0.0f);
}

f32 Time::beginFrame() {
    auto& state = getState();

    if (!state.initialized) {
        init();
    }

    TimePoint now = Clock::now();
    Duration elapsed = now - state.lastFrameTime;
    state.lastFrameTime = now;

    // Calculate raw delta time
    state.rawDeltaTime = static_cast<f32>(elapsed.count());

    // Clamp to max delta time to prevent spiral of death
    state.rawDeltaTime = std::min(state.rawDeltaTime, state.config.maxDeltaTime);

    // Add to smoothing buffer
    state.deltaSamples[state.sampleIndex] = state.rawDeltaTime;
    state.sampleIndex = (state.sampleIndex + 1) % state.deltaSamples.size();
    state.sampleCount =
        std::min(state.sampleCount + 1, static_cast<u32>(state.deltaSamples.size()));

    // Calculate smoothed delta
    state.smoothedDeltaTime = calculateSmoothedDelta(state);

    // Add to fixed timestep accumulator (using raw delta, not scaled)
    state.accumulator += state.rawDeltaTime;
    state.fixedUpdatesThisFrame = 0;

    // Update frame counter
    state.frameCount++;

    // Update FPS calculation
    state.fpsAccumulator += state.rawDeltaTime;
    state.fpsFrameCount++;
    if (state.fpsAccumulator >= 0.5f) {  // Update FPS every 0.5 seconds
        state.currentFPS = static_cast<f32>(state.fpsFrameCount) / state.fpsAccumulator;
        state.fpsAccumulator = 0.0f;
        state.fpsFrameCount = 0;
    }

    return state.rawDeltaTime;
}

bool Time::shouldFixedUpdate() {
    auto& state = getState();

    // Check if we've exceeded max updates (prevent lockup)
    if (state.fixedUpdatesThisFrame >= state.config.maxFixedUpdatesPerFrame) {
        // Drain remaining accumulator to prevent buildup
        state.accumulator = 0.0f;
        return false;
    }

    // Check if we have enough time accumulated
    if (state.accumulator >= state.config.fixedDeltaTime) {
        state.accumulator -= state.config.fixedDeltaTime;
        state.fixedUpdatesThisFrame++;
        return true;
    }

    return false;
}

f32 Time::getInterpolationAlpha() {
    auto& state = getState();
    return state.accumulator / state.config.fixedDeltaTime;
}

f32 Time::getDeltaTime() {
    auto& state = getState();
    return state.smoothedDeltaTime * state.timeScale;
}

f32 Time::getRawDeltaTime() {
    return getState().rawDeltaTime;
}

f32 Time::getFixedDeltaTime() {
    return getState().config.fixedDeltaTime;
}

f64 Time::getTimeSinceStart() {
    auto& state = getState();
    Duration elapsed = Clock::now() - state.startTime;
    return elapsed.count();
}

u64 Time::getFrameCount() {
    return getState().frameCount;
}

f32 Time::getFPS() {
    return getState().currentFPS;
}

void Time::setTimeScale(f32 scale) {
    getState().timeScale = std::max(0.0f, scale);
}

f32 Time::getTimeScale() {
    return getState().timeScale;
}

const TimeConfig& Time::getConfig() {
    return getState().config;
}

void Time::setConfig(const TimeConfig& config) {
    getState().config = config;
}

}  // namespace limbo
