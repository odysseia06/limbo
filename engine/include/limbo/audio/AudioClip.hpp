#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"

#include <vector>

namespace limbo {

/// Audio format specification
struct LIMBO_API AudioFormat {
    u32 sampleRate = 44100;
    u32 channels = 2;
    u32 bitsPerSample = 16;
};

/// Represents loaded audio data that can be played by AudioSource
class LIMBO_API AudioClip {
public:
    AudioClip() = default;
    ~AudioClip() = default;

    // Non-copyable, movable
    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;
    AudioClip(AudioClip&&) = default;
    AudioClip& operator=(AudioClip&&) = default;

    /// Load audio from file (WAV, MP3, FLAC, OGG supported)
    bool loadFromFile(const String& filepath);

    /// Load audio from memory buffer
    bool loadFromMemory(const void* data, usize size);

    /// Check if audio data is loaded
    bool isLoaded() const { return !m_samples.empty(); }

    /// Get audio format
    const AudioFormat& getFormat() const { return m_format; }

    /// Get sample data
    const std::vector<f32>& getSamples() const { return m_samples; }

    /// Get duration in seconds
    f32 getDuration() const;

    /// Get sample count
    usize getSampleCount() const { return m_samples.size(); }

    /// Get file path (if loaded from file)
    const String& getFilePath() const { return m_filepath; }

    /// Generate a test tone (sine wave)
    void generateTestTone(f32 frequency = 440.0f, f32 duration = 1.0f, f32 amplitude = 0.5f);

private:
    AudioFormat m_format;
    std::vector<f32> m_samples;  // Interleaved float samples
    String m_filepath;
};

}  // namespace limbo
