#include "limbo/render/common/Texture.hpp"
#include "limbo/core/Assert.hpp"

#include <glad/gl.h>
#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace limbo {

namespace {
GLenum toGLInternalFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::R8:
        return GL_R8;
    case TextureFormat::RG8:
        return GL_RG8;
    case TextureFormat::RGB8:
        return GL_RGB8;
    case TextureFormat::RGBA8:
        return GL_RGBA8;
    case TextureFormat::R16F:
        return GL_R16F;
    case TextureFormat::RG16F:
        return GL_RG16F;
    case TextureFormat::RGB16F:
        return GL_RGB16F;
    case TextureFormat::RGBA16F:
        return GL_RGBA16F;
    case TextureFormat::R32F:
        return GL_R32F;
    case TextureFormat::RG32F:
        return GL_RG32F;
    case TextureFormat::RGB32F:
        return GL_RGB32F;
    case TextureFormat::RGBA32F:
        return GL_RGBA32F;
    case TextureFormat::Depth24Stencil8:
        return GL_DEPTH24_STENCIL8;
    case TextureFormat::None:
        return 0;
    }
    return 0;
}

GLenum toGLFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::R8:
    case TextureFormat::R16F:
    case TextureFormat::R32F:
        return GL_RED;
    case TextureFormat::RG8:
    case TextureFormat::RG16F:
    case TextureFormat::RG32F:
        return GL_RG;
    case TextureFormat::RGB8:
    case TextureFormat::RGB16F:
    case TextureFormat::RGB32F:
        return GL_RGB;
    case TextureFormat::RGBA8:
    case TextureFormat::RGBA16F:
    case TextureFormat::RGBA32F:
        return GL_RGBA;
    case TextureFormat::Depth24Stencil8:
        return GL_DEPTH_STENCIL;
    case TextureFormat::None:
        return 0;
    }
    return 0;
}

GLenum toGLType(TextureFormat format) {
    switch (format) {
    case TextureFormat::R8:
    case TextureFormat::RG8:
    case TextureFormat::RGB8:
    case TextureFormat::RGBA8:
        return GL_UNSIGNED_BYTE;
    case TextureFormat::R16F:
    case TextureFormat::RG16F:
    case TextureFormat::RGB16F:
    case TextureFormat::RGBA16F:
    case TextureFormat::R32F:
    case TextureFormat::RG32F:
    case TextureFormat::RGB32F:
    case TextureFormat::RGBA32F:
        return GL_FLOAT;
    case TextureFormat::Depth24Stencil8:
        return GL_UNSIGNED_INT_24_8;
    case TextureFormat::None:
        return 0;
    }
    return 0;
}

GLenum toGLFilter(TextureFilter filter) {
    switch (filter) {
    case TextureFilter::Nearest:
        return GL_NEAREST;
    case TextureFilter::Linear:
        return GL_LINEAR;
    case TextureFilter::NearestMipmapNearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    case TextureFilter::LinearMipmapNearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case TextureFilter::NearestMipmapLinear:
        return GL_NEAREST_MIPMAP_LINEAR;
    case TextureFilter::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

GLenum toGLWrap(TextureWrap wrap) {
    switch (wrap) {
    case TextureWrap::Repeat:
        return GL_REPEAT;
    case TextureWrap::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case TextureWrap::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case TextureWrap::ClampToBorder:
        return GL_CLAMP_TO_BORDER;
    }
    return GL_REPEAT;
}
}  // namespace

Texture2D::~Texture2D() {
    destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_textureId(other.m_textureId), m_width(other.m_width), m_height(other.m_height),
      m_format(other.m_format) {
    other.m_textureId = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_format = TextureFormat::None;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        destroy();
        m_textureId = other.m_textureId;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;
        other.m_textureId = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_format = TextureFormat::None;
    }
    return *this;
}

Result<void, String> Texture2D::loadFromFile(const std::filesystem::path& path) {
    // Flip vertically for OpenGL coordinate system
    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (!data) {
        return unexpected<String>("Failed to load texture: " + path.string() + " - " +
                                  stbi_failure_reason());
    }

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
        return unexpected<String>("Unsupported number of channels: " + std::to_string(channels));
    }

    TextureSpec spec;
    spec.width = static_cast<u32>(width);
    spec.height = static_cast<u32>(height);
    spec.format = format;

    auto result = create(spec, data);
    stbi_image_free(data);

    if (result) {
        spdlog::debug("Texture loaded: {} ({}x{}, {} channels)", path.string(), width, height,
                      channels);
    }

    return result;
}

Result<void, String> Texture2D::create(const TextureSpec& spec, const void* data) {
    destroy();

    m_width = spec.width;
    m_height = spec.height;
    m_format = spec.format;

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureId);

    // Set texture parameters
    glTextureParameteri(m_textureId, GL_TEXTURE_MIN_FILTER,
                        static_cast<GLint>(toGLFilter(spec.minFilter)));
    glTextureParameteri(m_textureId, GL_TEXTURE_MAG_FILTER,
                        static_cast<GLint>(toGLFilter(spec.magFilter)));
    glTextureParameteri(m_textureId, GL_TEXTURE_WRAP_S, static_cast<GLint>(toGLWrap(spec.wrapS)));
    glTextureParameteri(m_textureId, GL_TEXTURE_WRAP_T, static_cast<GLint>(toGLWrap(spec.wrapT)));

    GLenum const internalFormat = toGLInternalFormat(spec.format);
    GLenum const dataFormat = toGLFormat(spec.format);
    GLenum const dataType = toGLType(spec.format);

    // Allocate storage
    u32 const mipLevels =
        spec.generateMipmaps
            ? static_cast<u32>(std::floor(std::log2(std::max(m_width, m_height)))) + 1
            : 1;

    glTextureStorage2D(m_textureId, static_cast<GLsizei>(mipLevels), internalFormat,
                       static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));

    if (data) {
        glTextureSubImage2D(m_textureId, 0, 0, 0, static_cast<GLsizei>(m_width),
                            static_cast<GLsizei>(m_height), dataFormat, dataType, data);

        if (spec.generateMipmaps) {
            glGenerateTextureMipmap(m_textureId);
        }
    }

    spdlog::debug("Texture2D created (ID: {}, {}x{})", m_textureId, m_width, m_height);
    return {};
}

void Texture2D::setData(const void* data, u32 size) {
    LIMBO_ASSERT(m_textureId != 0, "Texture not created");
    LIMBO_UNUSED(size);

    GLenum const dataFormat = toGLFormat(m_format);
    GLenum const dataType = toGLType(m_format);

    glTextureSubImage2D(m_textureId, 0, 0, 0, static_cast<GLsizei>(m_width),
                        static_cast<GLsizei>(m_height), dataFormat, dataType, data);
}

void Texture2D::bind(u32 slot) const {
    LIMBO_ASSERT(m_textureId != 0, "Texture not created");
    glBindTextureUnit(slot, m_textureId);
}

void Texture2D::unbind(u32 slot) {
    glBindTextureUnit(slot, 0);
}

void Texture2D::destroy() {
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
        spdlog::debug("Texture2D destroyed (ID: {})", m_textureId);
        m_textureId = 0;
    }
    m_width = 0;
    m_height = 0;
    m_format = TextureFormat::None;
}

}  // namespace limbo
