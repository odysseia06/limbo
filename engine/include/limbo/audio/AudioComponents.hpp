#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/audio/AudioSource.hpp"
#include "limbo/assets/AudioAsset.hpp"

namespace limbo {

/**
 * AudioSourceComponent - ECS component for audio playback
 *
 * Attach this to an entity to enable audio playback.
 * The AudioSystem will manage the underlying AudioSource.
 */
struct LIMBO_API AudioSourceComponent {
    /// The audio asset to play
    Shared<AudioAsset> audioAsset;

    /// Volume (0.0 to 1.0)
    f32 volume = 1.0f;

    /// Pitch multiplier (1.0 = normal)
    f32 pitch = 1.0f;

    /// Whether to loop playback
    bool loop = false;

    /// Whether to start playing automatically
    bool playOnStart = false;

    /// Whether this source is spatial (3D positioned audio)
    bool spatial = false;

    /// Runtime audio source (managed by AudioSystem)
    AudioSource* runtimeSource = nullptr;

    AudioSourceComponent() = default;
    explicit AudioSourceComponent(Shared<AudioAsset> asset, bool autoPlay = false)
        : audioAsset(std::move(asset)), playOnStart(autoPlay) {}
};

/**
 * AudioListenerComponent - ECS component for the audio listener (camera/player)
 *
 * There should typically be only one active listener in the scene.
 * For 2D games, this is usually attached to the camera entity.
 */
struct LIMBO_API AudioListenerComponent {
    /// Master volume for the listener
    f32 volume = 1.0f;

    /// Whether this listener is active
    bool active = true;

    AudioListenerComponent() = default;
};

}  // namespace limbo
