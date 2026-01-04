#pragma once

#include "limbo/assets/Asset.hpp"
#include "limbo/audio/AudioClip.hpp"

namespace limbo {

/**
 * AudioAsset - Managed audio asset
 *
 * Wraps an AudioClip and provides asset management features like
 * loading from disk and hot-reloading.
 */
class LIMBO_API AudioAsset : public Asset {
public:
    AudioAsset() = default;
    ~AudioAsset() override = default;

    [[nodiscard]] AssetType getType() const override { return AssetType::Audio; }

    /**
     * Get the underlying audio clip
     */
    [[nodiscard]] AudioClip* getClip() { return m_clip.get(); }
    [[nodiscard]] const AudioClip* getClip() const { return m_clip.get(); }

    /**
     * Get audio format
     */
    [[nodiscard]] const AudioFormat& getFormat() const {
        static AudioFormat const s_defaultFormat;
        return m_clip ? m_clip->getFormat() : s_defaultFormat;
    }

    /**
     * Get duration in seconds
     */
    [[nodiscard]] f32 getDuration() const { return m_clip ? m_clip->getDuration() : 0.0f; }

    /**
     * Get sample count
     */
    [[nodiscard]] usize getSampleCount() const { return m_clip ? m_clip->getSampleCount() : 0; }

protected:
    friend class AssetManager;

    bool load() override;
    void unload() override;

private:
    Unique<AudioClip> m_clip;
};

}  // namespace limbo
