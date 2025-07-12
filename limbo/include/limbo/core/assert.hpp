#pragma once
#if defined(LIMBO_ENABLE_ASSERTS)
#   include <limbo/core/log.hpp>
#   define LIMBO_ASSERT(cond, ...)                                \
        do {                                                      \
            if(!(cond)) {                                         \
                limbo::log::fatal("Assertion failed: {}", #cond); \
                std::terminate();                                 \
            }                                                     \
        } while(0)
#else
#   define LIMBO_ASSERT(cond, ...) ((void)0)
#endif
