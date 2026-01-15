#include "limbo/assets/TextureAsset.hpp"

#include "limbo/debug/Log.hpp"

#include <stb_image.h>

namespace limbo {

bool TextureAsset::load() {
    // Synchronous load: do both IO and GPU in one call
    if (!loadIO()) {
        return false;
    }
    return uploadGPU();
}

bool TextureAsset::loadIO() {
    // This can run on a worker thread - no OpenGL calls allowed

    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* data = stbi_load(getPath().string().c_str(), &width, &height, &channels, 0);
    if (!data) {
        setError("Failed to load texture: " + getPath().string() + " - " + stbi_failure_reason());
        LIMBO_LOG_ASSET_ERROR("Failed to load texture '{}': {}", getPath().string(),
                              stbi_failure_reason());
        return false;
    }

    // Determine format based on channels
    TextureFormat format;
    switch (channels) {
    case 1:
        format = TextureFormat::R8;
        break;
    case 2:
        format = TextureFormat::RG8;
        break;
    case 3:
        format = TextureFormat::RGB8;
        break;
    case 4:
        format = TextureFormat::RGBA8;
        break;
    default:
        stbi_image_free(data);
        setError("Unsupported number of channels: " + std::to_string(channels));
        LIMBO_LOG_ASSET_ERROR("Failed to load texture '{}': unsupported channels {}",
                              getPath().string(), channels);
        return false;
    }

    // Store decoded data for GPU upload
    usize const dataSize = static_cast<usize>(width) * static_cast<usize>(height) *
                           static_cast<usize>(channels);
    m_pendingData.assign(data, data + dataSize);
    stbi_image_free(data);

    m_pendingSpec.width = static_cast<u32>(width);
    m_pendingSpec.height = static_cast<u32>(height);
    m_pendingSpec.format = format;

    LIMBO_LOG_ASSET_DEBUG("Decoded texture: {} ({}x{}, {} channels)", getPath().string(), width,
                          height, channels);
    return true;
}

bool TextureAsset::uploadGPU() {
    // This must run on the main thread - creates OpenGL resources

    if (m_pendingData.empty()) {
        setError("No pending data to upload");
        LIMBO_LOG_ASSET_ERROR("TextureAsset::uploadGPU called without pending data");
        return false;
    }

    m_texture = make_unique<Texture2D>();
    auto result = m_texture->create(m_pendingSpec, m_pendingData.data());

    // Clear pending data regardless of success
    m_pendingData.clear();
    m_pendingData.shrink_to_fit();
    m_pendingSpec = {};

    if (!result) {
        setError(result.error());
        LIMBO_LOG_ASSET_ERROR("Failed to upload texture '{}' to GPU: {}", getPath().string(),
                              result.error());
        m_texture.reset();
        return false;
    }

    LIMBO_LOG_ASSET_DEBUG("Uploaded texture to GPU: {} ({}x{})", getPath().string(),
                          m_texture->getWidth(), m_texture->getHeight());
    return true;
}

void TextureAsset::unload() {
    m_texture.reset();
    m_pendingData.clear();
    m_pendingData.shrink_to_fit();
    m_pendingSpec = {};
}

}  // namespace limbo
