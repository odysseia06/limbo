#include "limbo/core/clock.hpp"
#include <format>

namespace limbo {

    std::chrono::nanoseconds Clock::now() noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            steady_time::now().time_since_epoch());
    }

    std::string Clock::iso_datetime()
    {
        const auto tp = sys_time::now();
        const auto zt = std::chrono::zoned_time{ std::chrono::current_zone(), tp };
        return std::format("{:%F %T}", zt);
    }

} // namespace limbo