#pragma once
#include <functional>
#include <string_view>
#include <limbo/core/event.hpp>

struct GLFWwindow;   // fwd

namespace limbo {

    class Platform {
    public:
        bool  init(int w, int h, std::string_view title);
        void  shutdown();

        bool  should_close() const;
        void  poll_events(const std::function<void(const Event&)>& sink);
        void  swap_buffers(bool vsync);
        double time_seconds() const;

    private:
        GLFWwindow* window_ = nullptr;
    };

} // namespace limbo
