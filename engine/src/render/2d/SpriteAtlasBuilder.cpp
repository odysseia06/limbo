#include "limbo/render/2d/SpriteAtlasBuilder.hpp"
#include "limbo/debug/Log.hpp"

#include <glad/gl.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cstring>
#include <numeric>

namespace limbo {

void SpriteAtlasBuilder::addSprite(const String& name, const std::filesystem::path& path,
                                   const glm::vec2& pivot) {
    AtlasInputSprite sprite;
    sprite.name = name;
    sprite.path = path;
    sprite.pivot = pivot;
    m_sprites.push_back(std::move(sprite));
}

void SpriteAtlasBuilder::addDirectory(const std::filesystem::path& directory, bool recursive,
                                      const std::vector<String>& extensions) {
    if (!std::filesystem::exists(directory)) {
        LIMBO_LOG_RENDER_WARN("SpriteAtlasBuilder: Directory does not exist: {}",
                              directory.string());
        return;
    }

    auto processEntry = [&](const std::filesystem::directory_entry& entry) {
        if (!entry.is_regular_file()) {
            return;
        }

        String ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool validExtension = extensions.empty();
        for (const auto& allowed : extensions) {
            if (ext == allowed) {
                validExtension = true;
                break;
            }
        }

        if (validExtension) {
            // Use filename without extension as sprite name
            String name = entry.path().stem().string();
            addSprite(name, entry.path());
        }
    };

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            processEntry(entry);
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            processEntry(entry);
        }
    }
}

void SpriteAtlasBuilder::clear() {
    m_sprites.clear();
}

bool SpriteAtlasBuilder::loadImages() {
    for (auto& sprite : m_sprites) {
        if (!std::filesystem::exists(sprite.path)) {
            LIMBO_LOG_RENDER_ERROR("SpriteAtlasBuilder: Image not found: {}", sprite.path.string());
            return false;
        }

        i32 width = 0;
        i32 height = 0;
        i32 channels = 0;

        // Force RGBA for consistency
        stbi_set_flip_vertically_on_load(false);  // We'll handle flipping in texture creation
        u8* data = stbi_load(sprite.path.string().c_str(), &width, &height, &channels, 4);

        if (data == nullptr) {
            LIMBO_LOG_RENDER_ERROR("SpriteAtlasBuilder: Failed to load image: {} - {}",
                                   sprite.path.string(), stbi_failure_reason());
            return false;
        }

        sprite.width = static_cast<u32>(width);
        sprite.height = static_cast<u32>(height);
        sprite.channels = 4;  // We forced RGBA

        usize size = static_cast<usize>(width) * static_cast<usize>(height) * 4;
        sprite.pixels.assign(data, data + size);

        stbi_image_free(data);
    }

    return true;
}

u32 SpriteAtlasBuilder::nextPowerOfTwo(u32 value) {
    if (value == 0) {
        return 1;
    }
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    return value;
}

glm::uvec2 SpriteAtlasBuilder::calculateAtlasSize(const AtlasBuildConfig& config) const {
    // Calculate total area needed
    u64 totalArea = 0;
    u32 maxWidth = 0;
    u32 maxHeight = 0;

    for (const auto& sprite : m_sprites) {
        u32 paddedWidth = sprite.width + config.padding * 2;
        u32 paddedHeight = sprite.height + config.padding * 2;
        totalArea += static_cast<u64>(paddedWidth) * static_cast<u64>(paddedHeight);
        maxWidth = std::max(maxWidth, paddedWidth);
        maxHeight = std::max(maxHeight, paddedHeight);
    }

    // Start with a square that could fit all sprites
    u32 side = static_cast<u32>(std::sqrt(static_cast<f64>(totalArea) * 1.2));  // 20% overhead
    side = std::max(side, std::max(maxWidth, maxHeight));

    if (config.powerOfTwo) {
        side = nextPowerOfTwo(side);
    }

    // Clamp to max dimensions
    u32 width = std::min(side, config.maxWidth);
    u32 height = std::min(side, config.maxHeight);

    return {width, height};
}

std::vector<PackRect> SpriteAtlasBuilder::packSprites(u32 atlasWidth, u32 atlasHeight,
                                                      const AtlasBuildConfig& config) {
    std::vector<PackRect> result;
    result.reserve(m_sprites.size());

    // MaxRects algorithm - maintain list of free rectangles
    std::vector<PackRect> freeRects;
    freeRects.push_back({0, 0, atlasWidth, atlasHeight, -1, false});

    // Sort sprites by area (largest first) for better packing
    std::vector<usize> sortedIndices(m_sprites.size());
    std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
    std::sort(sortedIndices.begin(), sortedIndices.end(), [this](usize a, usize b) {
        u64 areaA = static_cast<u64>(m_sprites[a].width) * static_cast<u64>(m_sprites[a].height);
        u64 areaB = static_cast<u64>(m_sprites[b].width) * static_cast<u64>(m_sprites[b].height);
        return areaA > areaB;
    });

    for (usize spriteIdx : sortedIndices) {
        const auto& sprite = m_sprites[spriteIdx];
        u32 spriteWidth = sprite.width + config.padding * 2;
        u32 spriteHeight = sprite.height + config.padding * 2;

        // Find best free rect (Best Short Side Fit heuristic)
        i32 bestRectIndex = -1;
        u32 bestShortSide = UINT32_MAX;
        bool bestRotated = false;

        for (usize i = 0; i < freeRects.size(); ++i) {
            const auto& freeRect = freeRects[i];

            // Try normal orientation
            if (spriteWidth <= freeRect.width && spriteHeight <= freeRect.height) {
                u32 leftoverHoriz = freeRect.width - spriteWidth;
                u32 leftoverVert = freeRect.height - spriteHeight;
                u32 shortSide = std::min(leftoverHoriz, leftoverVert);

                if (shortSide < bestShortSide) {
                    bestShortSide = shortSide;
                    bestRectIndex = static_cast<i32>(i);
                    bestRotated = false;
                }
            }

            // Try rotated orientation
            if (config.allowRotation && spriteHeight <= freeRect.width &&
                spriteWidth <= freeRect.height) {
                u32 leftoverHoriz = freeRect.width - spriteHeight;
                u32 leftoverVert = freeRect.height - spriteWidth;
                u32 shortSide = std::min(leftoverHoriz, leftoverVert);

                if (shortSide < bestShortSide) {
                    bestShortSide = shortSide;
                    bestRectIndex = static_cast<i32>(i);
                    bestRotated = true;
                }
            }
        }

        if (bestRectIndex < 0) {
            // Couldn't fit this sprite
            PackRect failed;
            failed.spriteIndex = static_cast<i32>(spriteIdx);
            failed.width = 0;
            failed.height = 0;
            result.push_back(failed);
            continue;
        }

        // Place the sprite
        PackRect& freeRect = freeRects[static_cast<usize>(bestRectIndex)];
        PackRect placed;
        placed.x = freeRect.x + config.padding;
        placed.y = freeRect.y + config.padding;
        placed.width = bestRotated ? sprite.height : sprite.width;
        placed.height = bestRotated ? sprite.width : sprite.height;
        placed.spriteIndex = static_cast<i32>(spriteIdx);
        placed.rotated = bestRotated;
        result.push_back(placed);

        // Split the free rect (Guillotine split)
        u32 placedWidth = bestRotated ? spriteHeight : spriteWidth;
        u32 placedHeight = bestRotated ? spriteWidth : spriteHeight;

        // Right split
        if (freeRect.width > placedWidth) {
            PackRect rightRect;
            rightRect.x = freeRect.x + placedWidth;
            rightRect.y = freeRect.y;
            rightRect.width = freeRect.width - placedWidth;
            rightRect.height = placedHeight;
            rightRect.spriteIndex = -1;
            if (rightRect.width > 0 && rightRect.height > 0) {
                freeRects.push_back(rightRect);
            }
        }

        // Bottom split
        if (freeRect.height > placedHeight) {
            PackRect bottomRect;
            bottomRect.x = freeRect.x;
            bottomRect.y = freeRect.y + placedHeight;
            bottomRect.width = freeRect.width;
            bottomRect.height = freeRect.height - placedHeight;
            bottomRect.spriteIndex = -1;
            if (bottomRect.width > 0 && bottomRect.height > 0) {
                freeRects.push_back(bottomRect);
            }
        }

        // Remove the used free rect
        freeRects.erase(freeRects.begin() + bestRectIndex);
    }

    return result;
}

Unique<Texture2D> SpriteAtlasBuilder::createTexture(const std::vector<PackRect>& packed,
                                                    u32 atlasWidth, u32 atlasHeight,
                                                    const AtlasBuildConfig& config) {
    // Create atlas pixel buffer
    std::vector<u8> atlasPixels(static_cast<usize>(atlasWidth) * atlasHeight * 4, 0);

    // Fill with background color
    u8 bgR = static_cast<u8>((config.backgroundColor >> 24) & 0xFF);
    u8 bgG = static_cast<u8>((config.backgroundColor >> 16) & 0xFF);
    u8 bgB = static_cast<u8>((config.backgroundColor >> 8) & 0xFF);
    u8 bgA = static_cast<u8>(config.backgroundColor & 0xFF);

    for (usize i = 0; i < atlasPixels.size(); i += 4) {
        atlasPixels[i + 0] = bgR;
        atlasPixels[i + 1] = bgG;
        atlasPixels[i + 2] = bgB;
        atlasPixels[i + 3] = bgA;
    }

    // Copy sprites to atlas
    for (const auto& rect : packed) {
        if (rect.spriteIndex < 0 || rect.width == 0) {
            continue;  // Failed to pack or free rect
        }

        const auto& sprite = m_sprites[static_cast<usize>(rect.spriteIndex)];

        if (rect.rotated) {
            // Copy with 90-degree rotation
            for (u32 srcY = 0; srcY < sprite.height; ++srcY) {
                for (u32 srcX = 0; srcX < sprite.width; ++srcX) {
                    u32 dstX = rect.x + srcY;
                    u32 dstY = rect.y + (sprite.width - 1 - srcX);

                    usize srcIdx = (static_cast<usize>(srcY) * sprite.width + srcX) * 4;
                    usize dstIdx = (static_cast<usize>(dstY) * atlasWidth + dstX) * 4;

                    atlasPixels[dstIdx + 0] = sprite.pixels[srcIdx + 0];
                    atlasPixels[dstIdx + 1] = sprite.pixels[srcIdx + 1];
                    atlasPixels[dstIdx + 2] = sprite.pixels[srcIdx + 2];
                    atlasPixels[dstIdx + 3] = sprite.pixels[srcIdx + 3];
                }
            }
        } else {
            // Copy without rotation
            for (u32 srcY = 0; srcY < sprite.height; ++srcY) {
                for (u32 srcX = 0; srcX < sprite.width; ++srcX) {
                    u32 dstX = rect.x + srcX;
                    u32 dstY = rect.y + srcY;

                    usize srcIdx = (static_cast<usize>(srcY) * sprite.width + srcX) * 4;
                    usize dstIdx = (static_cast<usize>(dstY) * atlasWidth + dstX) * 4;

                    atlasPixels[dstIdx + 0] = sprite.pixels[srcIdx + 0];
                    atlasPixels[dstIdx + 1] = sprite.pixels[srcIdx + 1];
                    atlasPixels[dstIdx + 2] = sprite.pixels[srcIdx + 2];
                    atlasPixels[dstIdx + 3] = sprite.pixels[srcIdx + 3];
                }
            }
        }
    }

    // Flip vertically for OpenGL
    std::vector<u8> flippedPixels(atlasPixels.size());
    usize rowSize = static_cast<usize>(atlasWidth) * 4;
    for (u32 y = 0; y < atlasHeight; ++y) {
        u32 srcRow = atlasHeight - 1 - y;
        std::memcpy(flippedPixels.data() + y * rowSize, atlasPixels.data() + srcRow * rowSize,
                    rowSize);
    }

    // Create texture
    TextureSpec spec;
    spec.width = atlasWidth;
    spec.height = atlasHeight;
    spec.format = TextureFormat::RGBA8;
    spec.generateMipmaps = config.generateMipmaps;
    spec.minFilter =
        config.generateMipmaps ? TextureFilter::LinearMipmapLinear : TextureFilter::Linear;
    spec.magFilter = TextureFilter::Linear;

    auto texture = std::make_unique<Texture2D>();
    auto result = texture->create(spec, flippedPixels.data());
    if (!result) {
        LIMBO_LOG_RENDER_ERROR("SpriteAtlasBuilder: Failed to create texture: {}", result.error());
        return nullptr;
    }

    return texture;
}

AtlasBuildResult SpriteAtlasBuilder::build(const AtlasBuildConfig& config) {
    AtlasBuildResult result;
    result.totalSprites = static_cast<u32>(m_sprites.size());

    if (m_sprites.empty()) {
        result.error = "No sprites to pack";
        return result;
    }

    // Load all images
    if (!loadImages()) {
        result.error = "Failed to load one or more images";
        return result;
    }

    // Calculate atlas size
    glm::uvec2 atlasSize = calculateAtlasSize(config);
    u32 atlasWidth = atlasSize.x;
    u32 atlasHeight = atlasSize.y;

    // Pack sprites
    std::vector<PackRect> packed = packSprites(atlasWidth, atlasHeight, config);

    // Check for overflow
    u64 usedArea = 0;
    for (const auto& rect : packed) {
        if (rect.spriteIndex >= 0) {
            if (rect.width == 0) {
                result.overflow.push_back(m_sprites[static_cast<usize>(rect.spriteIndex)].name);
            } else {
                result.packedSprites++;
                usedArea += static_cast<u64>(rect.width) * static_cast<u64>(rect.height);
            }
        }
    }

    if (!result.overflow.empty()) {
        LIMBO_LOG_RENDER_WARN("SpriteAtlasBuilder: {} sprites couldn't fit in atlas",
                              result.overflow.size());
    }

    // Calculate packing efficiency
    u64 totalArea = static_cast<u64>(atlasWidth) * static_cast<u64>(atlasHeight);
    result.packingEfficiency = static_cast<f32>(usedArea) / static_cast<f32>(totalArea);

    // Create the atlas
    result.atlas = std::make_unique<SpriteAtlas>();
    result.atlas->setSize(atlasWidth, atlasHeight);

    // Create texture
    result.atlas->setTexture(createTexture(packed, atlasWidth, atlasHeight, config));

    // Add regions
    for (const auto& rect : packed) {
        if (rect.spriteIndex < 0 || rect.width == 0) {
            continue;
        }

        const auto& sprite = m_sprites[static_cast<usize>(rect.spriteIndex)];

        SpriteRegion region;
        region.name = sprite.name;
        region.x = rect.x;
        region.y = rect.y;
        region.width = rect.width;
        region.height = rect.height;
        region.pivot = sprite.pivot;
        region.sourceFile = sprite.path.string();
        region.rotated = rect.rotated;

        // Calculate UV coordinates (note: texture is flipped for OpenGL)
        region.uvMin.x = static_cast<f32>(rect.x) / static_cast<f32>(atlasWidth);
        region.uvMin.y = static_cast<f32>(rect.y) / static_cast<f32>(atlasHeight);
        region.uvMax.x = static_cast<f32>(rect.x + rect.width) / static_cast<f32>(atlasWidth);
        region.uvMax.y = static_cast<f32>(rect.y + rect.height) / static_cast<f32>(atlasHeight);

        result.atlas->addRegion(region);
    }

    result.success = true;
    LIMBO_LOG_RENDER_INFO(
        "SpriteAtlasBuilder: Built {}x{} atlas with {} sprites ({:.1f}% efficiency)", atlasWidth,
        atlasHeight, result.packedSprites, result.packingEfficiency * 100.0f);

    return result;
}

bool SpriteAtlasBuilder::saveAtlas(const SpriteAtlas& atlas, const std::filesystem::path& atlasPath,
                                   const std::filesystem::path& texturePath) {
    // Save texture as PNG
    const Texture2D* texture = atlas.getTexture();
    if (texture == nullptr) {
        LIMBO_LOG_RENDER_ERROR("SpriteAtlasBuilder: Atlas has no texture");
        return false;
    }

    // Get texture data from GPU
    u32 width = texture->getWidth();
    u32 height = texture->getHeight();
    std::vector<u8> pixels(static_cast<usize>(width) * height * 4);

    glBindTexture(GL_TEXTURE_2D, texture->getNativeHandle());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Flip for file output (OpenGL is bottom-up)
    std::vector<u8> flippedPixels(pixels.size());
    usize rowSize = static_cast<usize>(width) * 4;
    for (u32 y = 0; y < height; ++y) {
        u32 srcRow = height - 1 - y;
        std::memcpy(flippedPixels.data() + y * rowSize, pixels.data() + srcRow * rowSize, rowSize);
    }

    // Create output directories if needed
    std::filesystem::create_directories(texturePath.parent_path());
    std::filesystem::create_directories(atlasPath.parent_path());

    // Save PNG
    i32 result = stbi_write_png(texturePath.string().c_str(), static_cast<i32>(width),
                                static_cast<i32>(height), 4, flippedPixels.data(),
                                static_cast<i32>(width * 4));
    if (result == 0) {
        LIMBO_LOG_RENDER_ERROR("SpriteAtlasBuilder: Failed to write texture: {}",
                               texturePath.string());
        return false;
    }

    // Save metadata with relative texture path
    String relativeTexturePath = texturePath.filename().string();
    if (!atlas.saveMetadata(atlasPath, relativeTexturePath)) {
        return false;
    }

    LIMBO_LOG_RENDER_INFO("SpriteAtlasBuilder: Saved atlas to {} and {}", atlasPath.string(),
                          texturePath.string());
    return true;
}

}  // namespace limbo
