#include <limbo/core/log.hpp>

int main()
{
    limbo::log::init(limbo::log::Level::Debug,
        limbo::log::Level::Trace);   // console, file

    limbo::log::info("Hello, {}!", "Limbo");
    limbo::log::warning("Everything's fine – probably.");

    limbo::log::shutdown();
}