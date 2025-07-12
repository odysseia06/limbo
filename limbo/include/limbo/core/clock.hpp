#pragma once
#include <chrono>
#include <string>

namespace limbo {

    struct Clock {
        using sys_time = std::chrono::system_clock;
        using steady_time = std::chrono::steady_clock;

        /// Monotonic time since program start (nanoseconds).
        static std::chrono::nanoseconds now()    noexcept;

        /// `"YYYY-MM-DD HH:MM:SS"`    — local wall-clock for logs, filenames, …
        static std::string iso_datetime();
    };

} // namespace limbo
