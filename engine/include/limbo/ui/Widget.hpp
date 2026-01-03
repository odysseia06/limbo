#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace limbo {

/**
 * Anchor - Defines how a widget is positioned relative to its parent
 */
enum class Anchor : u8 {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/**
 * WidgetState - Current interaction state of a widget
 */
enum class WidgetState : u8 {
    Normal,
    Hovered,
    Pressed,
    Disabled
};

// Forward declarations
class Widget;
class UICanvas;

/**
 * WidgetStyle - Visual styling for widgets
 */
struct WidgetStyle {
    glm::vec4 backgroundColor{0.2f, 0.2f, 0.2f, 0.8f};
    glm::vec4 borderColor{0.4f, 0.4f, 0.4f, 1.0f};
    glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 hoverColor{0.3f, 0.3f, 0.3f, 0.9f};
    glm::vec4 pressedColor{0.15f, 0.15f, 0.15f, 0.95f};
    glm::vec4 disabledColor{0.1f, 0.1f, 0.1f, 0.5f};
    f32 borderWidth = 1.0f;
    f32 cornerRadius = 0.0f;  // For future rounded corners
    f32 padding = 4.0f;
};

/**
 * Widget - Base class for all UI elements
 */
class LIMBO_API Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;
    
    // Non-copyable
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    
    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    /**
     * Update widget logic (input handling, animations)
     */
    virtual void update(f32 deltaTime);
    
    /**
     * Render the widget
     * @param screenSize Size of the screen/canvas in pixels
     */
    virtual void render(const glm::vec2& screenSize);
    
    // ========================================================================
    // Hierarchy
    // ========================================================================
    
    void addChild(Shared<Widget> child);
    void removeChild(Widget* child);
    void clearChildren();
    
    [[nodiscard]] Widget* getParent() const { return m_parent; }
    [[nodiscard]] const std::vector<Shared<Widget>>& getChildren() const { return m_children; }
    
    // ========================================================================
    // Layout
    // ========================================================================
    
    void setPosition(const glm::vec2& pos) { m_position = pos; m_dirty = true; }
    void setSize(const glm::vec2& size) { m_size = size; m_dirty = true; }
    void setAnchor(Anchor anchor) { m_anchor = anchor; m_dirty = true; }
    void setPivot(const glm::vec2& pivot) { m_pivot = pivot; m_dirty = true; }
    
    [[nodiscard]] const glm::vec2& getPosition() const { return m_position; }
    [[nodiscard]] const glm::vec2& getSize() const { return m_size; }
    [[nodiscard]] Anchor getAnchor() const { return m_anchor; }
    [[nodiscard]] const glm::vec2& getPivot() const { return m_pivot; }
    
    /**
     * Get computed screen-space bounds (min, max)
     */
    [[nodiscard]] glm::vec4 getScreenBounds(const glm::vec2& screenSize) const;
    
    /**
     * Get the anchor point position in parent space
     */
    [[nodiscard]] glm::vec2 getAnchorPosition(const glm::vec2& parentSize) const;
    
    // ========================================================================
    // State and Style
    // ========================================================================
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void setVisible(bool visible) { m_visible = visible; }
    void setInteractive(bool interactive) { m_interactive = interactive; }
    
    [[nodiscard]] bool isEnabled() const { return m_enabled; }
    [[nodiscard]] bool isVisible() const { return m_visible; }
    [[nodiscard]] bool isInteractive() const { return m_interactive; }
    [[nodiscard]] WidgetState getState() const { return m_state; }
    
    void setStyle(const WidgetStyle& style) { m_style = style; }
    [[nodiscard]] WidgetStyle& getStyle() { return m_style; }
    [[nodiscard]] const WidgetStyle& getStyle() const { return m_style; }
    
    // ========================================================================
    // Input Handling
    // ========================================================================
    
    /**
     * Check if point is inside this widget
     */
    [[nodiscard]] bool containsPoint(const glm::vec2& point, const glm::vec2& screenSize) const;
    
    /**
     * Handle mouse input
     */
    virtual bool onMouseMove(const glm::vec2& mousePos, const glm::vec2& screenSize);
    virtual bool onMouseDown(const glm::vec2& mousePos, const glm::vec2& screenSize);
    virtual bool onMouseUp(const glm::vec2& mousePos, const glm::vec2& screenSize);

protected:
    /**
     * Get current color based on state
     */
    [[nodiscard]] glm::vec4 getCurrentBackgroundColor() const;
    
    // Layout
    glm::vec2 m_position{0.0f};     // Position relative to anchor
    glm::vec2 m_size{100.0f, 30.0f};
    Anchor m_anchor = Anchor::TopLeft;
    glm::vec2 m_pivot{0.0f, 0.0f};  // 0,0 = top-left, 1,1 = bottom-right
    
    // State
    bool m_enabled = true;
    bool m_visible = true;
    bool m_interactive = true;
    bool m_dirty = true;
    WidgetState m_state = WidgetState::Normal;
    
    // Style
    WidgetStyle m_style;
    
    // Hierarchy
    Widget* m_parent = nullptr;
    std::vector<Shared<Widget>> m_children;
};

} // namespace limbo
