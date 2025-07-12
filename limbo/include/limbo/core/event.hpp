#pragma once
#include <cstdint>

namespace limbo {

    enum class EventType { None, KeyDown, KeyUp, WindowClose, WindowResize };

    struct KeyEvent {
        std::uint32_t key;   // GLFW keycodes
    };

    struct WindowResizeEvent {
        int width;
        int height;
    };

    struct Event {
        EventType type = EventType::None;
        union {
            KeyEvent           key;
            WindowResizeEvent  resize;
        };
    };

} // namespace limbo
