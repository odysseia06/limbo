#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "limbo/audio/AudioEngine.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace limbo {

// Static callback wrapper for miniaudio
static void dataCallback(ma_device* device, void* output, const void* /*input*/,
                         ma_uint32 frameCount) {
    auto* engine = static_cast<AudioEngine*>(device->pUserData);
    if (engine) {
        engine->audioCallback(output, frameCount);
    }
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init() {
    if (m_initialized) {
        spdlog::warn("AudioEngine already initialized");
        return true;
    }

    m_device = new ma_device();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = m_channels;
    config.sampleRate = m_sampleRate;
    config.dataCallback = dataCallback;
    config.pUserData = this;

    ma_result result = ma_device_init(nullptr, &config, m_device);
    if (result != MA_SUCCESS) {
        spdlog::error("Failed to initialize audio device: {}", static_cast<int>(result));
        delete m_device;
        m_device = nullptr;
        return false;
    }

    // Update with actual device settings
    m_sampleRate = m_device->sampleRate;
    m_channels = m_device->playback.channels;

    result = ma_device_start(m_device);
    if (result != MA_SUCCESS) {
        spdlog::error("Failed to start audio device: {}", static_cast<int>(result));
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
        return false;
    }

    m_initialized = true;
    spdlog::info("AudioEngine initialized ({}Hz, {} channels)", m_sampleRate, m_channels);
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_device) {
        ma_device_stop(m_device);
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(m_sourcesMutex);
        m_sources.clear();
    }

    m_initialized = false;
    spdlog::info("AudioEngine shutdown");
}

void AudioEngine::registerSource(AudioSource* source) {
    if (!source)
        return;

    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    auto it = std::find(m_sources.begin(), m_sources.end(), source);
    if (it == m_sources.end()) {
        m_sources.push_back(source);
    }
}

void AudioEngine::unregisterSource(AudioSource* source) {
    if (!source)
        return;

    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    auto it = std::find(m_sources.begin(), m_sources.end(), source);
    if (it != m_sources.end()) {
        m_sources.erase(it);
    }
}

void AudioEngine::setMasterVolume(f32 volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioEngine::audioCallback(void* output, u32 frameCount) {
    auto* outputBuffer = static_cast<f32*>(output);
    const u32 totalSamples = frameCount * m_channels;

    // Clear output buffer
    std::fill(outputBuffer, outputBuffer + totalSamples, 0.0f);

    std::lock_guard<std::mutex> lock(m_sourcesMutex);

    for (AudioSource* source : m_sources) {
        if (!source || !source->isPlaying()) {
            continue;
        }

        AudioClip* clip = source->getClip();
        if (!clip || !clip->isLoaded()) {
            continue;
        }

        const auto& samples = clip->getSamples();
        const auto& format = clip->getFormat();
        const usize clipSampleCount = samples.size();

        if (clipSampleCount == 0) {
            continue;
        }

        const f32 volume = source->getVolume() * m_masterVolume;
        usize samplePos = source->getSamplePosition();

        // Mix samples into output buffer
        for (u32 frame = 0; frame < frameCount; ++frame) {
            if (samplePos >= clipSampleCount) {
                if (source->isLooping()) {
                    samplePos = 0;
                } else {
                    source->stop();
                    break;
                }
            }

            // Handle channel conversion if needed
            for (u32 ch = 0; ch < m_channels; ++ch) {
                usize srcChannel = ch % format.channels;
                usize srcIndex = (samplePos / format.channels) * format.channels + srcChannel;

                if (srcIndex < clipSampleCount) {
                    outputBuffer[frame * m_channels + ch] += samples[srcIndex] * volume;
                }
            }

            samplePos += format.channels;
        }

        source->setSamplePosition(samplePos);
    }

    // Clamp output to prevent clipping
    for (u32 i = 0; i < totalSamples; ++i) {
        outputBuffer[i] = std::clamp(outputBuffer[i], -1.0f, 1.0f);
    }
}

// ============================================================================
// AudioClip implementation
// ============================================================================

bool AudioClip::loadFromFile(const String& filepath) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder decoder;

    ma_result result = ma_decoder_init_file(filepath.c_str(), &config, &decoder);
    if (result != MA_SUCCESS) {
        spdlog::error("Failed to open audio file: {}", filepath);
        return false;
    }

    // Get audio info
    m_format.sampleRate = decoder.outputSampleRate;
    m_format.channels = decoder.outputChannels;
    m_format.bitsPerSample = 32;  // f32 format

    // Get total frame count
    ma_uint64 totalFrames;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    if (result != MA_SUCCESS) {
        // Estimate for streaming formats
        totalFrames = static_cast<ma_uint64>(m_format.sampleRate * 600);  // Max 10 minutes
    }

    // Read all samples
    m_samples.resize(static_cast<usize>(totalFrames * m_format.channels));

    ma_uint64 framesRead;
    result = ma_decoder_read_pcm_frames(&decoder, m_samples.data(), totalFrames, &framesRead);

    // Resize to actual size
    m_samples.resize(static_cast<usize>(framesRead * m_format.channels));
    m_samples.shrink_to_fit();

    ma_decoder_uninit(&decoder);

    m_filepath = filepath;
    spdlog::debug("Loaded audio: {} ({} samples, {}Hz, {} channels)", filepath, m_samples.size(),
                  m_format.sampleRate, m_format.channels);

    return true;
}

bool AudioClip::loadFromMemory(const void* data, usize size) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder decoder;

    ma_result result = ma_decoder_init_memory(data, size, &config, &decoder);
    if (result != MA_SUCCESS) {
        spdlog::error("Failed to decode audio from memory");
        return false;
    }

    // Get audio info
    m_format.sampleRate = decoder.outputSampleRate;
    m_format.channels = decoder.outputChannels;
    m_format.bitsPerSample = 32;

    // Get total frame count
    ma_uint64 totalFrames;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    if (result != MA_SUCCESS) {
        totalFrames = static_cast<ma_uint64>(m_format.sampleRate * 600);
    }

    // Read all samples
    m_samples.resize(static_cast<usize>(totalFrames * m_format.channels));

    ma_uint64 framesRead;
    result = ma_decoder_read_pcm_frames(&decoder, m_samples.data(), totalFrames, &framesRead);

    m_samples.resize(static_cast<usize>(framesRead * m_format.channels));
    m_samples.shrink_to_fit();

    ma_decoder_uninit(&decoder);

    m_filepath = "<memory>";
    return true;
}

void AudioClip::generateTestTone(f32 frequency, f32 duration, f32 amplitude) {
    m_format.sampleRate = 44100;
    m_format.channels = 2;
    m_format.bitsPerSample = 32;

    const usize totalSamples =
        static_cast<usize>(duration * static_cast<f32>(m_format.sampleRate) * m_format.channels);
    m_samples.resize(totalSamples);

    const f32 twoPi = 6.28318530718f;

    for (usize i = 0; i < totalSamples / m_format.channels; ++i) {
        f32 t = static_cast<f32>(i) / static_cast<f32>(m_format.sampleRate);
        f32 sample = amplitude * std::sin(twoPi * frequency * t);

        // Apply fade in/out to avoid clicks
        f32 fadeTime = 0.01f;  // 10ms fade
        if (t < fadeTime) {
            sample *= t / fadeTime;
        } else if (t > duration - fadeTime) {
            sample *= (duration - t) / fadeTime;
        }

        // Write to both channels (stereo)
        m_samples[i * 2] = sample;
        m_samples[i * 2 + 1] = sample;
    }

    m_filepath = "<generated>";
    spdlog::debug("Generated test tone: {}Hz, {}s", frequency, duration);
}

f32 AudioClip::getDuration() const {
    if (m_samples.empty() || m_format.sampleRate == 0 || m_format.channels == 0) {
        return 0.0f;
    }
    return static_cast<f32>(m_samples.size()) /
           static_cast<f32>(m_format.sampleRate * m_format.channels);
}

// ============================================================================
// AudioSource implementation
// ============================================================================

void AudioSource::setClip(AudioClip* clip) {
    stop();
    m_clip = clip;
}

void AudioSource::play() {
    if (m_clip && m_clip->isLoaded()) {
        m_state = AudioState::Playing;
    }
}

void AudioSource::pause() {
    if (m_state == AudioState::Playing) {
        m_state = AudioState::Paused;
    }
}

void AudioSource::stop() {
    m_state = AudioState::Stopped;
    m_samplePosition = 0;
}

void AudioSource::setVolume(f32 volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioSource::setPitch(f32 pitch) {
    m_pitch = std::clamp(pitch, 0.1f, 4.0f);
}

void AudioSource::setLooping(bool loop) {
    m_looping = loop;
}

f32 AudioSource::getPlaybackPosition() const {
    if (!m_clip || !m_clip->isLoaded()) {
        return 0.0f;
    }
    const auto& format = m_clip->getFormat();
    return static_cast<f32>(m_samplePosition) /
           static_cast<f32>(format.sampleRate * format.channels);
}

void AudioSource::setPlaybackPosition(f32 position) {
    if (!m_clip || !m_clip->isLoaded()) {
        return;
    }
    const auto& format = m_clip->getFormat();
    m_samplePosition =
        static_cast<usize>(position * static_cast<f32>(format.sampleRate * format.channels));
    m_samplePosition = std::min(m_samplePosition, m_clip->getSampleCount());
}

void AudioSource::advanceSamplePosition(usize samples) {
    m_samplePosition += samples;
}

}  // namespace limbo
