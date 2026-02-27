#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

struct GLFWwindow;

namespace limbo {

struct WindowConfig {
    String title = "Limbo Engine";
    i32 width = 1280;
    i32 height = 720;
    bool resizable = true;
    bool vsync = true;
};

class LIMBO_API Window {
public:
    Window() = default;
    ~Window();

    LIMBO_NON_COPYABLE(Window);

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    [[nodiscard]] static Result<Window> create(const WindowConfig& config);

    void pollEvents();
    void swapBuffers();

    [[nodiscard]] bool shouldClose() const;
    void setShouldClose(bool value);

    [[nodiscard]] i32 getWidth() const { return m_width; }
    [[nodiscard]] i32 getHeight() const { return m_height; }
    [[nodiscard]] GLFWwindow* getNativeHandle() const { return m_window; }

    void setTitle(const String& title);

private:
    explicit Window(GLFWwindow* window, i32 width, i32 height);

    GLFWwindow* m_window = nullptr;
    i32 m_width = 0;
    i32 m_height = 0;
};

namespace platform {

LIMBO_API bool init();
LIMBO_API void shutdown();
LIMBO_API f64 getTime();

}  // namespace platform

}  // namespace limbo
