#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/audio/AudioClip.hpp"

namespace limbo {

/// Playback state for audio source
enum class AudioState : u8 { Stopped = 0, Playing, Paused };

/// Represents an audio playback source that can play AudioClips
class LIMBO_API AudioSource {
public:
    AudioSource() = default;
    ~AudioSource() = default;

    // Non-copyable, movable
    AudioSource(const AudioSource&) = delete;
    AudioSource& operator=(const AudioSource&) = delete;
    AudioSource(AudioSource&&) = default;
    AudioSource& operator=(AudioSource&&) = default;

    /// Set the audio clip to play
    void setClip(AudioClip* clip);

    /// Get current clip
    AudioClip* getClip() const { return m_clip; }

    /// Play the audio
    void play();

    /// Pause playback
    void pause();

    /// Stop playback and reset to beginning
    void stop();

    /// Get current playback state
    AudioState getState() const { return m_state; }

    /// Check if currently playing
    bool isPlaying() const { return m_state == AudioState::Playing; }

    /// Set volume (0.0 to 1.0)
    void setVolume(f32 volume);
    f32 getVolume() const { return m_volume; }

    /// Set pitch multiplier (1.0 = normal)
    void setPitch(f32 pitch);
    f32 getPitch() const { return m_pitch; }

    /// Set looping
    void setLooping(bool loop);
    bool isLooping() const { return m_looping; }

    /// Get current playback position in seconds
    f32 getPlaybackPosition() const;

    /// Set playback position in seconds
    void setPlaybackPosition(f32 position);

    /// Get current sample position (internal use)
    usize getSamplePosition() const { return m_samplePosition; }
    void setSamplePosition(usize pos) { m_samplePosition = pos; }

    /// Advance sample position (internal use by AudioEngine)
    void advanceSamplePosition(usize samples);

private:
    AudioClip* m_clip = nullptr;
    AudioState m_state = AudioState::Stopped;
    f32 m_volume = 1.0f;
    f32 m_pitch = 1.0f;
    bool m_looping = false;
    usize m_samplePosition = 0;
};

}  // namespace limbo
