#include "limbo/core/platform.hpp"
#include "limbo/core/log.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <mutex>

namespace limbo {
    static std::vector<Event> queuedEvents;
    static std::mutex         queueMutex;

    static Event translate_glfw_key(int key, int action)
    {
        Event e{};
        if (action == GLFW_PRESS)
            e.type = EventType::KeyDown;
        else if (action == GLFW_RELEASE)
            e.type = EventType::KeyUp;
        e.key.key = static_cast<std::uint32_t>(key);
        return e;
    }

    bool Platform::init(int w, int h, std::string_view title)
    {
        if (!glfwInit()) {
            limbo::log::error("GLFW init failed");
            return false;
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   // no GL context yet
        window_ = glfwCreateWindow(w, h, std::string(title).c_str(),
            nullptr, nullptr);
        if (!window_) {
            limbo::log::error("GLFW window creation failed");
            glfwTerminate();
            return false;
        }
        glfwSetKeyCallback(window_,
            [](GLFWwindow*, int key, int /*scancode*/, int action, int /*mods*/)
            {
                Event ev{};
                if (action == GLFW_PRESS)        ev.type = EventType::KeyDown;
                else if (action == GLFW_RELEASE) ev.type = EventType::KeyUp;
                else                             return;

                ev.key.key = static_cast<std::uint32_t>(key);

                // Buffer the event in a thread-local queue so poll_events() can forward it
                std::lock_guard lock(queueMutex);
                queuedEvents.push_back(ev);
            });
        return true;
    }

    void Platform::shutdown()
    {
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
    }

    bool Platform::should_close() const
    {
        return glfwWindowShouldClose(window_);
    }

    void Platform::poll_events(const std::function<void(const Event&)>& sink)
    {
        glfwPollEvents();

        std::lock_guard lock(queueMutex);
        for (auto& e : queuedEvents)
            sink(e);
        queuedEvents.clear();

        // Window close
        if (glfwWindowShouldClose(window_)) {
            Event e{};
            e.type = EventType::WindowClose;
            sink(e);
        }
    }

    void Platform::swap_buffers(bool /*vsync*/)
    {
        // No GL context yet; nothing to swap
    }

    double Platform::time_seconds() const
    {
        return glfwGetTime();
    }

} // namespace limbo
