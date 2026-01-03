#include "limbo/assets/AudioAsset.hpp"
#include <spdlog/spdlog.h>

namespace limbo {

bool AudioAsset::load() {
    m_clip = std::make_unique<AudioClip>();

    if (!m_clip->loadFromFile(getPath().string())) {
        setError("Failed to load audio: " + getPath().string());
        m_clip.reset();
        return false;
    }

    spdlog::debug("Loaded audio asset: {} ({}s, {}Hz)", 
        getPath().string(), m_clip->getDuration(), m_clip->getFormat().sampleRate);

    return true;
}

void AudioAsset::unload() {
    m_clip.reset();
}

} // namespace limbo
