#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/audio/AudioClip.hpp"
#include "limbo/audio/AudioSource.hpp"

#include <vector>
#include <mutex>

// Forward declare miniaudio types
struct ma_device;
struct ma_device_config;

namespace limbo {

/// Audio engine that manages audio playback using miniaudio
class LIMBO_API AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine();

    // Non-copyable
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /// Initialize the audio engine
    bool init();

    /// Shutdown the audio engine
    void shutdown();

    /// Check if initialized
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /// Register an audio source for playback
    void registerSource(AudioSource* source);

    /// Unregister an audio source
    void unregisterSource(AudioSource* source);

    /// Set master volume (0.0 to 1.0)
    void setMasterVolume(f32 volume);
    [[nodiscard]] f32 getMasterVolume() const { return m_masterVolume; }

    /// Get sample rate
    [[nodiscard]] u32 getSampleRate() const { return m_sampleRate; }

    /// Get channel count
    [[nodiscard]] u32 getChannels() const { return m_channels; }

    /// Audio callback (internal use)
    void audioCallback(void* output, u32 frameCount);

private:
    ma_device* m_device = nullptr;
    bool m_initialized = false;
    f32 m_masterVolume = 1.0f;
    u32 m_sampleRate = 44100;
    u32 m_channels = 2;

    std::vector<AudioSource*> m_sources;
    std::mutex m_sourcesMutex;
};

}  // namespace limbo
