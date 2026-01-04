#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/animation/Animation.hpp"

#include <unordered_map>

namespace limbo {

/**
 * AnimatorComponent - ECS component for sprite animation
 *
 * Manages multiple animation clips and handles transitions between them.
 */
struct LIMBO_API AnimatorComponent {
    /// Current animation state
    AnimationState currentState;

    /// Map of animation clips by name
    std::unordered_map<String, Shared<AnimationClip>> clips;

    /// Name of the current animation
    String currentClipName;

    /// Whether to auto-play on start
    bool playOnStart = true;

    /// Default animation to play
    String defaultClip;

    AnimatorComponent() = default;

    /**
     * Add an animation clip
     */
    void addClip(const String& name, Shared<AnimationClip> clip) {
        clips[name] = std::move(clip);
        if (defaultClip.empty()) {
            defaultClip = name;
        }
    }

    /**
     * Play an animation by name
     */
    bool play(const String& name) {
        auto it = clips.find(name);
        if (it == clips.end()) {
            return false;
        }
        currentClipName = name;
        currentState.setClip(it->second.get());
        currentState.play();
        return true;
    }

    /**
     * Get the current sprite frame for rendering
     */
    const SpriteFrame* getCurrentFrame() const { return currentState.getCurrentSpriteFrame(); }

    /**
     * Check if an animation exists
     */
    bool hasClip(const String& name) const { return clips.contains(name); }
};

}  // namespace limbo
