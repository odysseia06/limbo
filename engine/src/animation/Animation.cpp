#include "limbo/animation/Animation.hpp"
#include <spdlog/spdlog.h>

namespace limbo {

// ============================================================================
// AnimationClip
// ============================================================================

AnimationClip::AnimationClip(const String& name) : m_name(name) {}

void AnimationClip::addFrame(u32 frameIndex, f32 duration) {
    AnimationFrame frame;
    frame.frameIndex = frameIndex;
    frame.duration = duration;
    m_frames.push_back(frame);
}

void AnimationClip::addFrameRange(u32 startFrame, u32 endFrame, f32 frameDuration) {
    for (u32 i = startFrame; i <= endFrame; ++i) {
        addFrame(i, frameDuration);
    }
}

const AnimationFrame& AnimationClip::getFrame(usize index) const {
    static AnimationFrame defaultFrame;
    if (index >= m_frames.size()) {
        return defaultFrame;
    }
    return m_frames[index];
}

f32 AnimationClip::getTotalDuration() const {
    f32 total = 0.0f;
    for (const auto& frame : m_frames) {
        total += frame.duration;
    }
    return total / m_speed;
}

void AnimationClip::clear() {
    m_frames.clear();
}

// ============================================================================
// AnimationState
// ============================================================================

void AnimationState::setClip(AnimationClip* clip) {
    m_clip = clip;
    stop();
}

bool AnimationState::update(f32 deltaTime) {
    if (!m_playing || !m_clip || m_clip->getFrameCount() == 0) {
        return false;
    }

    const f32 speed = m_clip->getSpeed();
    m_frameTime += deltaTime * speed;

    const AnimationFrame& currentFrame = m_clip->getFrame(m_currentFrame);

    while (m_frameTime >= currentFrame.duration) {
        m_frameTime -= currentFrame.duration;
        advanceFrame();

        if (m_finished) {
            return false;
        }
    }

    m_time += deltaTime * speed;
    return true;
}

void AnimationState::play() {
    m_playing = true;
    m_finished = false;
    m_time = 0.0f;
    m_frameTime = 0.0f;
    m_currentFrame = 0;
    m_reverse = false;
}

void AnimationState::pause() {
    m_playing = false;
}

void AnimationState::resume() {
    if (!m_finished) {
        m_playing = true;
    }
}

void AnimationState::stop() {
    m_playing = false;
    m_finished = false;
    m_time = 0.0f;
    m_frameTime = 0.0f;
    m_currentFrame = 0;
    m_reverse = false;
}

u32 AnimationState::getCurrentFrameIndex() const {
    if (!m_clip || m_clip->getFrameCount() == 0) {
        return 0;
    }
    return m_clip->getFrame(m_currentFrame).frameIndex;
}

const SpriteFrame* AnimationState::getCurrentSpriteFrame() const {
    if (!m_clip || !m_clip->getSpriteSheet()) {
        return nullptr;
    }
    u32 frameIndex = getCurrentFrameIndex();
    return &m_clip->getSpriteSheet()->getFrame(frameIndex);
}

f32 AnimationState::getNormalizedTime() const {
    if (!m_clip) {
        return 0.0f;
    }
    f32 totalDuration = m_clip->getTotalDuration();
    if (totalDuration <= 0.0f) {
        return 0.0f;
    }
    return m_time / totalDuration;
}

void AnimationState::setNormalizedTime(f32 t) {
    if (!m_clip || m_clip->getFrameCount() == 0) {
        return;
    }

    t = glm::clamp(t, 0.0f, 1.0f);
    f32 totalDuration = m_clip->getTotalDuration();
    f32 targetTime = t * totalDuration;

    // Find the frame at this time
    f32 accumulatedTime = 0.0f;
    for (usize i = 0; i < m_clip->getFrameCount(); ++i) {
        const AnimationFrame& frame = m_clip->getFrame(i);
        if (accumulatedTime + frame.duration > targetTime) {
            m_currentFrame = i;
            m_frameTime = targetTime - accumulatedTime;
            m_time = targetTime;
            return;
        }
        accumulatedTime += frame.duration;
    }

    // At the end
    m_currentFrame = m_clip->getFrameCount() - 1;
    m_frameTime = 0.0f;
    m_time = totalDuration;
}

void AnimationState::advanceFrame() {
    if (!m_clip || m_clip->getFrameCount() == 0) {
        return;
    }

    usize frameCount = m_clip->getFrameCount();
    usize prevFrame = m_currentFrame;

    switch (m_clip->getPlayMode()) {
    case AnimationPlayMode::Once:
        if (m_currentFrame + 1 >= frameCount) {
            m_finished = true;
            m_playing = false;
            if (m_onComplete) {
                m_onComplete();
            }
        } else {
            m_currentFrame++;
        }
        break;

    case AnimationPlayMode::Loop:
        m_currentFrame = (m_currentFrame + 1) % frameCount;
        break;

    case AnimationPlayMode::PingPong:
        if (m_reverse) {
            if (m_currentFrame == 0) {
                m_reverse = false;
                m_currentFrame = 1;
            } else {
                m_currentFrame--;
            }
        } else {
            if (m_currentFrame + 1 >= frameCount) {
                m_reverse = true;
                m_currentFrame = frameCount > 1 ? frameCount - 2 : 0;
            } else {
                m_currentFrame++;
            }
        }
        break;

    case AnimationPlayMode::ClampForever:
        if (m_currentFrame + 1 < frameCount) {
            m_currentFrame++;
        } else {
            m_finished = true;
            // Keep playing (for ClampForever)
        }
        break;
    }

    if (m_currentFrame != prevFrame && m_onFrameChange) {
        m_onFrameChange(getCurrentFrameIndex());
    }
}

}  // namespace limbo
