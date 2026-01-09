#include "limbo/debug/Debug.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo::debug {

void init() {
    // Initialize the logging system with categories
    limbo::log::init();
}

void shutdown() {
    // Shutdown the logging system
    limbo::log::shutdown();
}

}  // namespace limbo::debug
