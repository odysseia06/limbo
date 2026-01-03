#include "limbo/ui/Widget.hpp"
#include "limbo/render/Renderer2D.hpp"

#include <algorithm>

namespace limbo {

void Widget::update(f32 deltaTime) {
    (void)deltaTime;
    
    // Update children
    for (auto& child : m_children) {
        if (child->isEnabled()) {
            child->update(deltaTime);
        }
    }
}

void Widget::render(const glm::vec2& screenSize) {
    if (!m_visible) return;
    
    // Default rendering: draw background quad
    glm::vec4 bounds = getScreenBounds(screenSize);
    glm::vec3 pos((bounds.x + bounds.z) * 0.5f, (bounds.y + bounds.w) * 0.5f, 0.0f);
    glm::vec2 size(bounds.z - bounds.x, bounds.w - bounds.y);
    
    Renderer2D::drawQuad(pos, size, getCurrentBackgroundColor());
    
    // Draw border if width > 0
    if (m_style.borderWidth > 0.0f) {
        // Top border
        Renderer2D::drawQuad(
            glm::vec3(pos.x, bounds.w - m_style.borderWidth * 0.5f, 0.01f),
            glm::vec2(size.x, m_style.borderWidth),
            m_style.borderColor
        );
        // Bottom border
        Renderer2D::drawQuad(
            glm::vec3(pos.x, bounds.y + m_style.borderWidth * 0.5f, 0.01f),
            glm::vec2(size.x, m_style.borderWidth),
            m_style.borderColor
        );
        // Left border
        Renderer2D::drawQuad(
            glm::vec3(bounds.x + m_style.borderWidth * 0.5f, pos.y, 0.01f),
            glm::vec2(m_style.borderWidth, size.y),
            m_style.borderColor
        );
        // Right border
        Renderer2D::drawQuad(
            glm::vec3(bounds.z - m_style.borderWidth * 0.5f, pos.y, 0.01f),
            glm::vec2(m_style.borderWidth, size.y),
            m_style.borderColor
        );
    }
    
    // Render children
    for (auto& child : m_children) {
        child->render(screenSize);
    }
}

void Widget::addChild(Shared<Widget> child) {
    child->m_parent = this;
    m_children.push_back(std::move(child));
}

void Widget::removeChild(Widget* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const Shared<Widget>& w) { return w.get() == child; });
    
    if (it != m_children.end()) {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

void Widget::clearChildren() {
    for (auto& child : m_children) {
        child->m_parent = nullptr;
    }
    m_children.clear();
}

glm::vec4 Widget::getScreenBounds(const glm::vec2& screenSize) const {
    glm::vec2 parentSize = screenSize;
    glm::vec2 parentPos{0.0f};
    
    if (m_parent) {
        glm::vec4 parentBounds = m_parent->getScreenBounds(screenSize);
        parentPos = glm::vec2(parentBounds.x, parentBounds.y);
        parentSize = glm::vec2(parentBounds.z - parentBounds.x, parentBounds.w - parentBounds.y);
        
        // Children are positioned from parent's top-left with Y going down
        // Position is offset from top-left of parent
        glm::vec2 topLeft(parentBounds.x + m_position.x, 
                          parentBounds.w - m_position.y - m_size.y);
        return glm::vec4(topLeft.x, topLeft.y, topLeft.x + m_size.x, topLeft.y + m_size.y);
    }
    
    // Root widgets use anchor system
    // Get anchor position in parent space (screen coords: 0,0 at bottom-left)
    glm::vec2 anchorPos = getAnchorPosition(parentSize);
    
    // Position offset depends on anchor type
    // For top anchors, Y position goes DOWN (negative in screen coords)
    // For bottom anchors, Y position goes UP (positive in screen coords)
    glm::vec2 offset = m_position;
    
    // Adjust Y direction based on anchor
    if (m_anchor == Anchor::TopLeft || m_anchor == Anchor::TopCenter || m_anchor == Anchor::TopRight) {
        offset.y = -offset.y - m_size.y;  // Subtract size to position from top edge
    }
    if (m_anchor == Anchor::BottomRight || m_anchor == Anchor::CenterRight || m_anchor == Anchor::TopRight) {
        offset.x = -offset.x - m_size.x;  // Subtract size to position from right edge
    }
    
    // Calculate position with pivot offset
    glm::vec2 pivotOffset = m_size * m_pivot;
    glm::vec2 bottomLeft = parentPos + anchorPos + offset - pivotOffset;
    
    return glm::vec4(bottomLeft.x, bottomLeft.y, bottomLeft.x + m_size.x, bottomLeft.y + m_size.y);
}

glm::vec2 Widget::getAnchorPosition(const glm::vec2& parentSize) const {
    switch (m_anchor) {
        case Anchor::TopLeft:      return glm::vec2(0.0f, parentSize.y);
        case Anchor::TopCenter:    return glm::vec2(parentSize.x * 0.5f, parentSize.y);
        case Anchor::TopRight:     return glm::vec2(parentSize.x, parentSize.y);
        case Anchor::CenterLeft:   return glm::vec2(0.0f, parentSize.y * 0.5f);
        case Anchor::Center:       return glm::vec2(parentSize.x * 0.5f, parentSize.y * 0.5f);
        case Anchor::CenterRight:  return glm::vec2(parentSize.x, parentSize.y * 0.5f);
        case Anchor::BottomLeft:   return glm::vec2(0.0f, 0.0f);
        case Anchor::BottomCenter: return glm::vec2(parentSize.x * 0.5f, 0.0f);
        case Anchor::BottomRight:  return glm::vec2(parentSize.x, 0.0f);
    }
    return glm::vec2(0.0f);
}

bool Widget::containsPoint(const glm::vec2& point, const glm::vec2& screenSize) const {
    glm::vec4 bounds = getScreenBounds(screenSize);
    return point.x >= bounds.x && point.x <= bounds.z &&
           point.y >= bounds.y && point.y <= bounds.w;
}

bool Widget::onMouseMove(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    if (!m_enabled || !m_interactive) return false;
    
    bool isHovered = containsPoint(mousePos, screenSize);
    
    if (isHovered && m_state != WidgetState::Pressed) {
        m_state = WidgetState::Hovered;
    } else if (!isHovered && m_state == WidgetState::Hovered) {
        m_state = WidgetState::Normal;
    }
    
    // Propagate to children
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseMove(mousePos, screenSize)) {
            return true;
        }
    }
    
    return isHovered;
}

bool Widget::onMouseDown(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    if (!m_enabled || !m_interactive) return false;
    
    // Check children first (reverse order for proper z-ordering)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseDown(mousePos, screenSize)) {
            return true;
        }
    }
    
    if (containsPoint(mousePos, screenSize)) {
        m_state = WidgetState::Pressed;
        return true;
    }
    
    return false;
}

bool Widget::onMouseUp(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    if (!m_enabled || !m_interactive) return false;
    
    bool wasPressed = (m_state == WidgetState::Pressed);
    
    // Check children first
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseUp(mousePos, screenSize)) {
            return true;
        }
    }
    
    if (wasPressed) {
        m_state = containsPoint(mousePos, screenSize) ? WidgetState::Hovered : WidgetState::Normal;
        return containsPoint(mousePos, screenSize);
    }
    
    return false;
}

glm::vec4 Widget::getCurrentBackgroundColor() const {
    if (!m_enabled) return m_style.disabledColor;
    
    switch (m_state) {
        case WidgetState::Hovered: return m_style.hoverColor;
        case WidgetState::Pressed: return m_style.pressedColor;
        case WidgetState::Disabled: return m_style.disabledColor;
        default: return m_style.backgroundColor;
    }
}

} // namespace limbo
