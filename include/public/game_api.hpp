#pragma once
#include <cstdint>

extern "C" {

    // ─── 8 MiB flat memory block (adjust later) ─────────────────────────────
    struct GameMemory
    {
        std::uint8_t permanent[8 * 1024 * 1024];
    };

    // ─── function table exported by the DLL ────────────────────────────────
    struct GameExports
    {
        void (*update)(GameMemory*, double dt);
        void (*shutdown)(GameMemory*);
    };

    // exported symbol name resolved at runtime
    using GameBootstrapFn = GameExports * (*)(GameMemory*);
} // extern "C"

