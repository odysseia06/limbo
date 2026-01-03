#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <filesystem>

namespace limbo {

enum class TextureFormat : u8 {
    None = 0,
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    Depth24Stencil8
};

enum class TextureFilter : u8 {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureWrap : u8 {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

struct TextureSpec {
    u32 width = 1;
    u32 height = 1;
    TextureFormat format = TextureFormat::RGBA8;
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureWrap wrapS = TextureWrap::Repeat;
    TextureWrap wrapT = TextureWrap::Repeat;
    bool generateMipmaps = true;
};

class LIMBO_API Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();

    // Non-copyable
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // Movable
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    // Load texture from file
    [[nodiscard]] Result<void, String> loadFromFile(const std::filesystem::path& path);

    // Create texture from raw data
    [[nodiscard]] Result<void, String> create(const TextureSpec& spec, const void* data = nullptr);

    // Update texture data
    void setData(const void* data, u32 size);

    // Bind texture to a slot
    void bind(u32 slot = 0) const;
    static void unbind(u32 slot = 0);

    [[nodiscard]] bool isValid() const { return m_textureId != 0; }
    [[nodiscard]] u32 getWidth() const { return m_width; }
    [[nodiscard]] u32 getHeight() const { return m_height; }
    [[nodiscard]] TextureFormat getFormat() const { return m_format; }
    [[nodiscard]] u32 getNativeHandle() const { return m_textureId; }

private:
    void destroy();

    u32 m_textureId = 0;
    u32 m_width = 0;
    u32 m_height = 0;
    TextureFormat m_format = TextureFormat::None;
};

} // namespace limbo
