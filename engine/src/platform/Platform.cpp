#include "limbo/platform/Platform.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <fmt/core.h>

namespace limbo {

// Window implementation

Window::Window(GLFWwindow* window, i32 width, i32 height)
    : m_window(window), m_width(width), m_height(height) {}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

Window::Window(Window&& other) noexcept
    : m_window(other.m_window), m_width(other.m_width), m_height(other.m_height) {
    other.m_window = nullptr;
    other.m_width = 0;
    other.m_height = 0;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        m_window = other.m_window;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_window = nullptr;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

Result<Window> Window::create(const WindowConfig& config) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

#ifdef LIMBO_PLATFORM_MACOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

#ifdef LIMBO_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    GLFWwindow* window =
        glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);

    if (!window) {
        return unexpected<String>("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(config.vsync ? 1 : 0);

    // Set up resize callback
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
        LIMBO_UNUSED(win);
        spdlog::debug("Window resized to {}x{}", width, height);
    });

    spdlog::info(fmt::runtime("Window created: {} ({}x{})"), config.title, config.width,
                 config.height);

    return Window(window, config.width, config.height);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window) != 0;
}

void Window::setShouldClose(bool value) {
    glfwSetWindowShouldClose(m_window, value ? GLFW_TRUE : GLFW_FALSE);
}

// Platform functions

namespace platform {

static bool s_initialized = false;

static void glfwErrorCallback(int error, const char* description) {
    spdlog::error(fmt::runtime("GLFW Error ({}): {}"), error, description);
}

bool init() {
    if (s_initialized) {
        return true;
    }

    glfwSetErrorCallback(glfwErrorCallback);

    if (glfwInit() == GLFW_FALSE) {
        spdlog::critical("Failed to initialize GLFW");
        return false;
    }

    spdlog::info(fmt::runtime("GLFW initialized: {}"), glfwGetVersionString());
    s_initialized = true;
    return true;
}

void shutdown() {
    if (s_initialized) {
        glfwTerminate();
        s_initialized = false;
        spdlog::info("GLFW terminated");
    }
}

f64 getTime() {
    return glfwGetTime();
}

}  // namespace platform

}  // namespace limbo
