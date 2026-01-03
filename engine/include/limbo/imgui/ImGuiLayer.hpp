#pragma once

#include "limbo/core/Base.hpp"

// Forward declarations
struct GLFWwindow;

namespace limbo {

/**
 * ImGuiLayer - Manages Dear ImGui integration
 *
 * Handles initialization, frame management, and rendering of ImGui.
 * Should be initialized after the render context and shut down before it.
 *
 * Usage:
 *   ImGuiLayer imgui;
 *   imgui.init(window);
 *   
 *   // In render loop:
 *   imgui.beginFrame();
 *   ImGui::Begin("Debug");
 *   ImGui::Text("Hello!");
 *   ImGui::End();
 *   imgui.endFrame();
 *   
 *   imgui.shutdown();
 */
class LIMBO_API ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    // Non-copyable
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    /**
     * Initialize ImGui with GLFW window and OpenGL context
     * @param window GLFW window handle
     * @return true on success
     */
    bool init(GLFWwindow* window);

    /**
     * Shutdown ImGui and free resources
     */
    void shutdown();

    /**
     * Begin a new ImGui frame
     * Call this at the start of your UI rendering
     */
    void beginFrame();

    /**
     * End the ImGui frame and render
     * Call this after all ImGui calls are done
     */
    void endFrame();

    /**
     * Check if ImGui is initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * Enable or disable ImGui rendering
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * Check if ImGui is enabled
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * Set the dark color theme (default)
     */
    void setDarkTheme();

    /**
     * Set the light color theme
     */
    void setLightTheme();

    /**
     * Set a custom Limbo-styled theme
     */
    void setLimboTheme();

private:
    bool m_initialized = false;
    bool m_enabled = true;
};

} // namespace limbo
