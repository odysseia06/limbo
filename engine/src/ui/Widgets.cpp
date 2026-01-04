#include "limbo/ui/Widgets.hpp"
#include "limbo/render/2d/Renderer2D.hpp"

namespace limbo {

// ============================================================================
// Panel
// ============================================================================

void Panel::render(const glm::vec2& screenSize) {
    if (!m_visible) {
        return;
    }

    // Render panel background
    Widget::render(screenSize);
}

// ============================================================================
// Label
// ============================================================================

void Label::render(const glm::vec2& screenSize) {
    if (!m_visible) {
        return;
    }

    // For now, just render a simple indicator showing the text area
    // Full text rendering would require a font system
    glm::vec4 const bounds = getScreenBounds(screenSize);
    glm::vec3 const pos((bounds.x + bounds.z) * 0.5f, (bounds.y + bounds.w) * 0.5f, 0.0f);
    glm::vec2 const size(bounds.z - bounds.x, bounds.w - bounds.y);

    // Draw a simple representation (text color bar)
    if (!m_text.empty()) {
        // Draw a colored bar to represent text
        f32 const textWidth = std::min(size.x * 0.8f, static_cast<f32>(m_text.length()) * 6.0f);
        Renderer2D::drawQuad(glm::vec3(bounds.x + size.x * 0.1f + textWidth * 0.5f, pos.y, 0.02f),
                             glm::vec2(textWidth, size.y * 0.6f), m_style.textColor);
    }

    // Render children
    for (auto& child : m_children) {
        child->render(screenSize);
    }
}

// ============================================================================
// Button
// ============================================================================

bool Button::onMouseUp(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    bool const wasPressed = (m_state == WidgetState::Pressed);
    bool const result = Widget::onMouseUp(mousePos, screenSize);

    // Fire click callback if button was pressed and released over it
    if (wasPressed && containsPoint(mousePos, screenSize) && m_onClick) {
        m_onClick();
    }

    return result;
}

void Button::render(const glm::vec2& screenSize) {
    if (!m_visible) {
        return;
    }

    // Render button background
    glm::vec4 const bounds = getScreenBounds(screenSize);
    glm::vec3 const pos((bounds.x + bounds.z) * 0.5f, (bounds.y + bounds.w) * 0.5f, 0.0f);
    glm::vec2 const size(bounds.z - bounds.x, bounds.w - bounds.y);

    // Background
    Renderer2D::drawQuad(pos, size, getCurrentBackgroundColor());

    // Border
    if (m_style.borderWidth > 0.0f) {
        // Top
        Renderer2D::drawQuad(glm::vec3(pos.x, bounds.w - m_style.borderWidth * 0.5f, 0.01f),
                             glm::vec2(size.x, m_style.borderWidth), m_style.borderColor);
        // Bottom
        Renderer2D::drawQuad(glm::vec3(pos.x, bounds.y + m_style.borderWidth * 0.5f, 0.01f),
                             glm::vec2(size.x, m_style.borderWidth), m_style.borderColor);
        // Left
        Renderer2D::drawQuad(glm::vec3(bounds.x + m_style.borderWidth * 0.5f, pos.y, 0.01f),
                             glm::vec2(m_style.borderWidth, size.y), m_style.borderColor);
        // Right
        Renderer2D::drawQuad(glm::vec3(bounds.z - m_style.borderWidth * 0.5f, pos.y, 0.01f),
                             glm::vec2(m_style.borderWidth, size.y), m_style.borderColor);
    }

    // Text representation
    if (!m_text.empty()) {
        f32 const textWidth = std::min(size.x * 0.7f, static_cast<f32>(m_text.length()) * 6.0f);
        Renderer2D::drawQuad(glm::vec3(pos.x, pos.y, 0.02f), glm::vec2(textWidth, size.y * 0.4f),
                             m_style.textColor);
    }

    // Render children
    for (auto& child : m_children) {
        child->render(screenSize);
    }
}

// ============================================================================
// ProgressBar
// ============================================================================

void ProgressBar::render(const glm::vec2& screenSize) {
    if (!m_visible) {
        return;
    }

    glm::vec4 const bounds = getScreenBounds(screenSize);
    glm::vec3 const pos((bounds.x + bounds.z) * 0.5f, (bounds.y + bounds.w) * 0.5f, 0.0f);
    glm::vec2 const size(bounds.z - bounds.x, bounds.w - bounds.y);

    // Background
    Renderer2D::drawQuad(pos, size, m_style.backgroundColor);

    // Fill bar
    if (m_progress > 0.0f) {
        f32 const fillWidth = (size.x - m_style.borderWidth * 2.0f) * m_progress;
        f32 const fillHeight = size.y - m_style.borderWidth * 2.0f;
        glm::vec3 const fillPos(bounds.x + m_style.borderWidth + fillWidth * 0.5f, pos.y, 0.01f);
        Renderer2D::drawQuad(fillPos, glm::vec2(fillWidth, fillHeight), m_fillColor);
    }

    // Border
    if (m_style.borderWidth > 0.0f) {
        // Top
        Renderer2D::drawQuad(glm::vec3(pos.x, bounds.w - m_style.borderWidth * 0.5f, 0.02f),
                             glm::vec2(size.x, m_style.borderWidth), m_style.borderColor);
        // Bottom
        Renderer2D::drawQuad(glm::vec3(pos.x, bounds.y + m_style.borderWidth * 0.5f, 0.02f),
                             glm::vec2(size.x, m_style.borderWidth), m_style.borderColor);
        // Left
        Renderer2D::drawQuad(glm::vec3(bounds.x + m_style.borderWidth * 0.5f, pos.y, 0.02f),
                             glm::vec2(m_style.borderWidth, size.y), m_style.borderColor);
        // Right
        Renderer2D::drawQuad(glm::vec3(bounds.z - m_style.borderWidth * 0.5f, pos.y, 0.02f),
                             glm::vec2(m_style.borderWidth, size.y), m_style.borderColor);
    }
}

// ============================================================================
// Image
// ============================================================================

void Image::render(const glm::vec2& screenSize) {
    if (!m_visible) {
        return;
    }

    glm::vec4 const bounds = getScreenBounds(screenSize);
    glm::vec3 const pos((bounds.x + bounds.z) * 0.5f, (bounds.y + bounds.w) * 0.5f, 0.0f);
    glm::vec2 const size(bounds.z - bounds.x, bounds.w - bounds.y);

    if (m_texture) {
        Renderer2D::drawQuad(pos, size, *m_texture, 1.0f, m_style.backgroundColor);
    } else {
        Renderer2D::drawQuad(pos, size, m_style.backgroundColor);
    }

    // Render children
    for (auto& child : m_children) {
        child->render(screenSize);
    }
}

}  // namespace limbo
