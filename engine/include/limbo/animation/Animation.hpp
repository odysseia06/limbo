#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/animation/SpriteSheet.hpp"

#include <vector>
#include <functional>

namespace limbo {

/**
 * Animation playback mode
 */
enum class AnimationPlayMode : u8 {
    Once,         // Play once and stop
    Loop,         // Loop continuously
    PingPong,     // Play forward then backward
    ClampForever  // Play once and hold last frame
};

/**
 * Animation frame with timing information
 */
struct LIMBO_API AnimationFrame {
    u32 frameIndex = 0;   // Index into sprite sheet
    f32 duration = 0.1f;  // Duration in seconds
};

/**
 * AnimationClip - A sequence of frames that form an animation
 *
 * Represents a single animation (e.g., "walk", "jump", "idle")
 */
class LIMBO_API AnimationClip {
public:
    AnimationClip() = default;
    explicit AnimationClip(const String& name);
    ~AnimationClip() = default;

    /**
     * Set animation name
     */
    void setName(const String& name) { m_name = name; }
    const String& getName() const { return m_name; }

    /**
     * Set the sprite sheet this animation uses
     */
    void setSpriteSheet(SpriteSheet* sheet) { m_spriteSheet = sheet; }
    SpriteSheet* getSpriteSheet() const { return m_spriteSheet; }

    /**
     * Add a frame to the animation
     */
    void addFrame(u32 frameIndex, f32 duration = 0.1f);

    /**
     * Add frames from a range (for sequential frames)
     */
    void addFrameRange(u32 startFrame, u32 endFrame, f32 frameDuration = 0.1f);

    /**
     * Get frame at index
     */
    const AnimationFrame& getFrame(usize index) const;

    /**
     * Get frame count
     */
    usize getFrameCount() const { return m_frames.size(); }

    /**
     * Get total animation duration
     */
    f32 getTotalDuration() const;

    /**
     * Set playback mode
     */
    void setPlayMode(AnimationPlayMode mode) { m_playMode = mode; }
    AnimationPlayMode getPlayMode() const { return m_playMode; }

    /**
     * Set playback speed multiplier
     */
    void setSpeed(f32 speed) { m_speed = speed; }
    f32 getSpeed() const { return m_speed; }

    /**
     * Clear all frames
     */
    void clear();

    /**
     * Check if animation is valid
     */
    bool isValid() const { return m_spriteSheet != nullptr && !m_frames.empty(); }

private:
    String m_name;
    SpriteSheet* m_spriteSheet = nullptr;
    std::vector<AnimationFrame> m_frames;
    AnimationPlayMode m_playMode = AnimationPlayMode::Loop;
    f32 m_speed = 1.0f;
};

/**
 * AnimationState - Runtime playback state for an animation
 */
class LIMBO_API AnimationState {
public:
    AnimationState() = default;
    ~AnimationState() = default;

    /**
     * Set the animation clip to play
     */
    void setClip(AnimationClip* clip);
    AnimationClip* getClip() const { return m_clip; }

    /**
     * Update animation (call each frame)
     * @return true if animation is still playing
     */
    bool update(f32 deltaTime);

    /**
     * Play from beginning
     */
    void play();

    /**
     * Pause playback
     */
    void pause();

    /**
     * Resume playback
     */
    void resume();

    /**
     * Stop and reset to beginning
     */
    void stop();

    /**
     * Check if playing
     */
    bool isPlaying() const { return m_playing; }

    /**
     * Check if animation has finished (for non-looping)
     */
    bool isFinished() const { return m_finished; }

    /**
     * Get current frame index (into sprite sheet)
     */
    u32 getCurrentFrameIndex() const;

    /**
     * Get current sprite frame from the sprite sheet
     */
    const SpriteFrame* getCurrentSpriteFrame() const;

    /**
     * Get normalized playback time (0-1)
     */
    f32 getNormalizedTime() const;

    /**
     * Set normalized playback time (0-1)
     */
    void setNormalizedTime(f32 t);

    /**
     * Set callback for when animation completes (non-looping)
     */
    void setOnComplete(std::function<void()> callback) { m_onComplete = std::move(callback); }

    /**
     * Set callback for each frame change
     */
    void setOnFrameChange(std::function<void(u32)> callback) {
        m_onFrameChange = std::move(callback);
    }

private:
    void advanceFrame();

    AnimationClip* m_clip = nullptr;
    f32 m_time = 0.0f;
    f32 m_frameTime = 0.0f;
    usize m_currentFrame = 0;
    bool m_playing = false;
    bool m_finished = false;
    bool m_reverse = false;  // For ping-pong mode

    std::function<void()> m_onComplete;
    std::function<void(u32)> m_onFrameChange;
};

}  // namespace limbo
