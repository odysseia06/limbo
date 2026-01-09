#include "limbo/graphics/Framebuffer.hpp"

#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace limbo {

Framebuffer::Framebuffer(const FramebufferSpec& spec) : m_spec(spec) {
    invalidate();
}

Framebuffer::~Framebuffer() {
    cleanup();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_framebufferId(other.m_framebufferId), m_colorAttachment(other.m_colorAttachment),
      m_depthAttachment(other.m_depthAttachment), m_spec(other.m_spec) {
    other.m_framebufferId = 0;
    other.m_colorAttachment = 0;
    other.m_depthAttachment = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_framebufferId = other.m_framebufferId;
        m_colorAttachment = other.m_colorAttachment;
        m_depthAttachment = other.m_depthAttachment;
        m_spec = other.m_spec;
        other.m_framebufferId = 0;
        other.m_colorAttachment = 0;
        other.m_depthAttachment = 0;
    }
    return *this;
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glViewport(0, 0, static_cast<GLsizei>(m_spec.width), static_cast<GLsizei>(m_spec.height));
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::resize(u32 width, u32 height) {
    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        spdlog::warn("Invalid framebuffer size: {}x{}", width, height);
        return;
    }

    m_spec.width = width;
    m_spec.height = height;
    invalidate();
}

void Framebuffer::invalidate() {
    cleanup();

    // Create framebuffer
    glGenFramebuffers(1, &m_framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

    // Create color attachment texture
    glGenTextures(1, &m_colorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_colorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(m_spec.width),
                 static_cast<GLsizei>(m_spec.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorAttachment,
                           0);

    // Create depth attachment
    glGenTextures(1, &m_depthAttachment);
    glBindTexture(GL_TEXTURE_2D, m_depthAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(m_spec.width),
                 static_cast<GLsizei>(m_spec.height), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                 nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                           m_depthAttachment, 0);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    spdlog::debug("Framebuffer created (ID: {}, {}x{})", m_framebufferId, m_spec.width,
                  m_spec.height);
}

void Framebuffer::cleanup() {
    if (m_framebufferId != 0) {
        glDeleteFramebuffers(1, &m_framebufferId);
        m_framebufferId = 0;
    }
    if (m_colorAttachment != 0) {
        glDeleteTextures(1, &m_colorAttachment);
        m_colorAttachment = 0;
    }
    if (m_depthAttachment != 0) {
        glDeleteTextures(1, &m_depthAttachment);
        m_depthAttachment = 0;
    }
}

}  // namespace limbo
