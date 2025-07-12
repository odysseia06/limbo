#pragma once
#include <limbo/core/event.hpp>

namespace limbo {

    class Application {
    public:
        virtual ~Application() = default;
        virtual void on_update(double dt) = 0;
        virtual void on_event(const Event& e) { (void)e; }
    };

} // namespace limbo