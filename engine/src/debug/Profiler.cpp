#include "limbo/debug/Profiler.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <stack>

namespace limbo::profiler {

namespace {

// Thread-local sample stack for nested samples
struct ThreadSampleStack {
    std::stack<u32> sampleIndices;  // Stack of indices into current frame's samples
};

thread_local ThreadSampleStack t_sampleStack;

// Global profiler state
struct ProfilerState {
    bool initialized = false;
    bool enabled = true;

    u32 maxSamplesPerFrame = 4096;
    u32 historySize = 120;

    // Current frame being recorded
    FrameData currentFrame;
    u32 currentDepth = 0;

    // Completed frame history (ring buffer)
    std::vector<FrameData> history;
    u32 historyIndex = 0;

    // Captured frame for detailed analysis
    FrameData capturedFrame;
    bool hasCapturedFrame = false;

    // Frame counter
    u64 frameNumber = 0;

    // Mutex for thread safety when accessing shared state
    std::mutex mutex;

    // Main thread ID for validation
    std::thread::id mainThreadId;
};

ProfilerState& getState() {
    static ProfilerState state;
    return state;
}

}  // namespace

void Profiler::init(u32 maxSamplesPerFrame, u32 historyFrames) {
    auto& state = getState();

    if (state.initialized) {
        spdlog::warn("Profiler already initialized");
        return;
    }

    state.maxSamplesPerFrame = maxSamplesPerFrame;
    state.historySize = historyFrames;

    // Pre-allocate current frame samples
    state.currentFrame.samples.reserve(maxSamplesPerFrame);

    // Pre-allocate history
    state.history.resize(historyFrames);
    for (auto& frame : state.history) {
        frame.samples.reserve(maxSamplesPerFrame);
    }

    state.historyIndex = 0;
    state.frameNumber = 0;
    state.mainThreadId = std::this_thread::get_id();
    state.initialized = true;

    spdlog::debug("Profiler initialized (maxSamples={}, history={})", maxSamplesPerFrame,
                  historyFrames);
}

void Profiler::shutdown() {
    auto& state = getState();

    if (!state.initialized) {
        return;
    }

    state.currentFrame.clear();
    state.history.clear();
    state.capturedFrame.clear();
    state.hasCapturedFrame = false;
    state.initialized = false;

    spdlog::debug("Profiler shutdown");
}

void Profiler::beginFrame() {
    auto& state = getState();

    if (!state.initialized || !state.enabled) {
        return;
    }

    // Reset current frame
    state.currentFrame.clear();
    state.currentFrame.samples.reserve(state.maxSamplesPerFrame);
    state.currentFrame.frameNumber = state.frameNumber;
    state.currentFrame.frameStartTime = getTimestamp();
    state.currentDepth = 0;

    // Clear thread-local sample stack
    while (!t_sampleStack.sampleIndices.empty()) {
        t_sampleStack.sampleIndices.pop();
    }
}

void Profiler::endFrame() {
    auto& state = getState();

    if (!state.initialized || !state.enabled) {
        return;
    }

    state.currentFrame.frameEndTime = getTimestamp();

    // Store in history ring buffer
    {
        std::lock_guard<std::mutex> lock(state.mutex);

        state.history[state.historyIndex] = state.currentFrame;
        state.historyIndex = (state.historyIndex + 1) % state.historySize;
    }

    state.frameNumber++;
}

void Profiler::beginSample(const char* name) {
    auto& state = getState();

    if (!state.initialized || !state.enabled) {
        return;
    }

    // Check capacity
    if (state.currentFrame.samples.size() >= state.maxSamplesPerFrame) {
        // Log warning once per frame to avoid spam
        static u64 lastWarningFrame = 0;
        if (lastWarningFrame != state.frameNumber) {
            spdlog::warn("Profiler sample buffer full (max {})", state.maxSamplesPerFrame);
            lastWarningFrame = state.frameNumber;
        }
        return;
    }

    ProfilerSample sample;
    sample.name = name;
    sample.startTime = getTimestamp();
    sample.depth = state.currentDepth;
    sample.threadId = std::this_thread::get_id();

    // Set parent index (0 means no parent)
    if (!t_sampleStack.sampleIndices.empty()) {
        sample.parentIndex = t_sampleStack.sampleIndices.top() + 1;  // 1-indexed
    } else {
        sample.parentIndex = 0;
    }

    // Add to current frame
    u32 const sampleIndex = static_cast<u32>(state.currentFrame.samples.size());
    state.currentFrame.samples.push_back(sample);

    // Push onto stack
    t_sampleStack.sampleIndices.push(sampleIndex);
    state.currentDepth++;
}

void Profiler::endSample() {
    auto& state = getState();

    if (!state.initialized || !state.enabled) {
        return;
    }

    if (t_sampleStack.sampleIndices.empty()) {
        spdlog::warn("Profiler::endSample() called without matching beginSample()");
        return;
    }

    u32 const sampleIndex = t_sampleStack.sampleIndices.top();
    t_sampleStack.sampleIndices.pop();

    if (sampleIndex < state.currentFrame.samples.size()) {
        state.currentFrame.samples[sampleIndex].endTime = getTimestamp();
    }

    if (state.currentDepth > 0) {
        state.currentDepth--;
    }
}

void Profiler::captureFrame() {
    auto& state = getState();

    if (!state.initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(state.mutex);

    // Get the most recent completed frame from history
    u32 const lastIndex = (state.historyIndex + state.historySize - 1) % state.historySize;
    state.capturedFrame = state.history[lastIndex];
    state.hasCapturedFrame = true;
}

const FrameData* Profiler::getCapturedFrame() {
    auto& state = getState();

    if (!state.initialized || !state.hasCapturedFrame) {
        return nullptr;
    }

    return &state.capturedFrame;
}

const FrameData* Profiler::getLastFrame() {
    auto& state = getState();

    if (!state.initialized || state.history.empty()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(state.mutex);

    u32 const lastIndex = (state.historyIndex + state.historySize - 1) % state.historySize;
    return &state.history[lastIndex];
}

const std::vector<FrameData>& Profiler::getHistory() {
    return getState().history;
}

u64 Profiler::getFrameNumber() {
    return getState().frameNumber;
}

bool Profiler::exportToCSV(const char* filepath) {
    auto& state = getState();

    if (!state.initialized || !state.hasCapturedFrame) {
        spdlog::error("No captured frame to export");
        return false;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open file for CSV export: {}", filepath);
        return false;
    }

    // Write header
    file << "Name,Depth,Parent,StartTime(us),EndTime(us),Duration(us),Duration(ms)\n";

    // Write samples
    for (const auto& sample : state.capturedFrame.samples) {
        f64 const startUs =
            static_cast<f64>(sample.startTime - state.capturedFrame.frameStartTime) / 1000.0;
        f64 const endUs =
            static_cast<f64>(sample.endTime - state.capturedFrame.frameStartTime) / 1000.0;
        f64 const durationUs = sample.getDurationUs();
        f64 const durationMs = sample.getDurationMs();

        file << sample.name << "," << sample.depth << "," << sample.parentIndex << "," << startUs
             << "," << endUs << "," << durationUs << "," << durationMs << "\n";
    }

    file.close();
    spdlog::info("Exported profiler data to: {}", filepath);
    return true;
}

void Profiler::setEnabled(bool enabled) {
    getState().enabled = enabled;
}

bool Profiler::isEnabled() {
    return getState().enabled;
}

bool Profiler::isInitialized() {
    return getState().initialized;
}

u64 Profiler::getTimestamp() {
    using namespace std::chrono;
    return static_cast<u64>(
        duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count());
}

}  // namespace limbo::profiler
