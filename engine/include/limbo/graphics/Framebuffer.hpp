#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>

namespace limbo {

/**
 * Framebuffer specification for creation
 */
struct FramebufferSpec {
    u32 width = 1280;
    u32 height = 720;
    u32 samples = 1;  // MSAA samples (1 = no multisampling)
    bool swapChainTarget = false;
};

/**
 * Framebuffer - Offscreen render target
 * 
 * Used for rendering scenes to textures for display in ImGui viewports,
 * post-processing, or other offscreen rendering needs.
 */
class LIMBO_API Framebuffer {
public:
    Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();

    // Non-copyable
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Movable
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    /**
     * Bind this framebuffer as the render target
     */
    void bind();

    /**
     * Unbind and restore default framebuffer
     */
    void unbind();

    /**
     * Clear the framebuffer with a color
     */
    void clear(float r = 0.1f, float g = 0.1f, float b = 0.1f, float a = 1.0f);

    /**
     * Resize the framebuffer
     */
    void resize(u32 width, u32 height);

    /**
     * Get the color attachment texture ID (for ImGui::Image)
     */
    [[nodiscard]] u32 getColorAttachmentId() const { return m_colorAttachment; }

    /**
     * Get framebuffer dimensions
     */
    [[nodiscard]] u32 getWidth() const { return m_spec.width; }
    [[nodiscard]] u32 getHeight() const { return m_spec.height; }

    /**
     * Get the specification
     */
    [[nodiscard]] const FramebufferSpec& getSpec() const { return m_spec; }

private:
    void invalidate();
    void cleanup();

    u32 m_framebufferId = 0;
    u32 m_colorAttachment = 0;
    u32 m_depthAttachment = 0;
    FramebufferSpec m_spec;
};

}  // namespace limbo
