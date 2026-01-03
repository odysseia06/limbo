#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/ui/Widget.hpp"
#include "limbo/ecs/System.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

/**
 * UICanvas - A container for UI widgets that renders in screen space
 *
 * The canvas manages a hierarchy of widgets and handles input routing.
 */
class LIMBO_API UICanvas {
public:
    UICanvas() = default;
    ~UICanvas() = default;

    /**
     * Add a root widget to the canvas
     */
    void addWidget(Shared<Widget> widget);

    /**
     * Remove a widget from the canvas
     */
    void removeWidget(Widget* widget);

    /**
     * Clear all widgets
     */
    void clear();

    /**
     * Update all widgets
     */
    void update(f32 deltaTime);

    /**
     * Render all widgets
     * Note: Call this between Renderer2D::beginScene and endScene with a screen-space camera
     */
    void render(const glm::vec2& screenSize);

    /**
     * Handle mouse movement
     */
    void onMouseMove(const glm::vec2& mousePos, const glm::vec2& screenSize);

    /**
     * Handle mouse button press
     */
    void onMouseDown(const glm::vec2& mousePos, const glm::vec2& screenSize);

    /**
     * Handle mouse button release
     */
    void onMouseUp(const glm::vec2& mousePos, const glm::vec2& screenSize);

    /**
     * Get all root widgets
     */
    [[nodiscard]] const std::vector<Shared<Widget>>& getWidgets() const { return m_widgets; }

    /**
     * Enable/disable the canvas
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

private:
    std::vector<Shared<Widget>> m_widgets;
    bool m_enabled = true;
};

/**
 * UICanvasComponent - Attaches a UI canvas to an entity
 */
struct LIMBO_API UICanvasComponent {
    Shared<UICanvas> canvas;
    bool screenSpace = true;  // If false, renders in world space (billboarded)

    UICanvasComponent() : canvas(std::make_shared<UICanvas>()) {}
    explicit UICanvasComponent(Shared<UICanvas> c) : canvas(std::move(c)) {}
};

/**
 * UISystem - Updates and handles input for UI canvases
 */
class LIMBO_API UISystem : public System {
public:
    UISystem() = default;

    void setScreenSize(const glm::vec2& size) { m_screenSize = size; }

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;

    /**
     * Render all UI canvases (call during render phase)
     */
    void render(World& world);

private:
    glm::vec2 m_screenSize{1280.0f, 720.0f};
};

}  // namespace limbo
