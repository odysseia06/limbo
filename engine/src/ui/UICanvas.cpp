#include "limbo/ui/UICanvas.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/platform/Input.hpp"
#include "limbo/render/Renderer2D.hpp"
#include "limbo/render/Camera.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace limbo {

// ============================================================================
// UICanvas
// ============================================================================

void UICanvas::addWidget(Shared<Widget> widget) {
    m_widgets.push_back(std::move(widget));
}

void UICanvas::removeWidget(Widget* widget) {
    auto it = std::find_if(m_widgets.begin(), m_widgets.end(),
                           [widget](const Shared<Widget>& w) { return w.get() == widget; });

    if (it != m_widgets.end()) {
        m_widgets.erase(it);
    }
}

void UICanvas::clear() {
    m_widgets.clear();
}

void UICanvas::update(f32 deltaTime) {
    if (!m_enabled)
        return;

    for (auto& widget : m_widgets) {
        if (widget->isEnabled()) {
            widget->update(deltaTime);
        }
    }
}

void UICanvas::render(const glm::vec2& screenSize) {
    if (!m_enabled)
        return;

    for (auto& widget : m_widgets) {
        if (widget->isVisible()) {
            widget->render(screenSize);
        }
    }
}

void UICanvas::onMouseMove(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    if (!m_enabled)
        return;

    // Process in reverse order (top widgets first)
    for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
        if ((*it)->onMouseMove(mousePos, screenSize)) {
            break;
        }
    }
}

void UICanvas::onMouseDown(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    if (!m_enabled)
        return;

    for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
        if ((*it)->onMouseDown(mousePos, screenSize)) {
            break;
        }
    }
}

void UICanvas::onMouseUp(const glm::vec2& mousePos, const glm::vec2& screenSize) {
    if (!m_enabled)
        return;

    for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
        if ((*it)->onMouseUp(mousePos, screenSize)) {
            break;
        }
    }
}

// ============================================================================
// UISystem
// ============================================================================

void UISystem::onAttach(World& world) {
    (void)world;
    spdlog::debug("UISystem attached");
}

void UISystem::update(World& world, f32 deltaTime) {
    // Get mouse position
    glm::vec2 mousePos(Input::getMouseX(), m_screenSize.y - Input::getMouseY());

    // Update all canvases
    world.each<UICanvasComponent>(
        [this, deltaTime, &mousePos](World::EntityId, UICanvasComponent& canvasComp) {
            if (!canvasComp.canvas)
                return;

            // Update widgets
            canvasComp.canvas->update(deltaTime);

            // Handle input
            canvasComp.canvas->onMouseMove(mousePos, m_screenSize);

            if (Input::isMouseButtonPressed(MouseButton::Left)) {
                canvasComp.canvas->onMouseDown(mousePos, m_screenSize);
            }
            if (Input::isMouseButtonReleased(MouseButton::Left)) {
                canvasComp.canvas->onMouseUp(mousePos, m_screenSize);
            }
        });
}

void UISystem::onDetach(World& world) {
    (void)world;
    spdlog::debug("UISystem detached");
}

void UISystem::render(World& world) {
    // Create a screen-space orthographic camera
    OrthographicCamera uiCamera(0.0f, m_screenSize.x, 0.0f, m_screenSize.y);

    // Begin UI rendering
    Renderer2D::beginScene(uiCamera);

    world.each<UICanvasComponent>([this](World::EntityId, UICanvasComponent& canvasComp) {
        if (canvasComp.canvas && canvasComp.screenSpace) {
            canvasComp.canvas->render(m_screenSize);
        }
    });

    Renderer2D::endScene();
}

}  // namespace limbo
