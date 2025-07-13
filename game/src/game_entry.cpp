#include "public/game_api.hpp"
#include <limbo/core/log.hpp>

static GameExports exports;

static void update(GameMemory*, double dt)
{
    static double acc = 0.0;
    acc += dt;
    if (acc > 1.0)
    {
        limbo::log::info("DLL update tick – dt ≈ {}", dt);
        acc = 0.0;
    }
}

static void shutdown(GameMemory*)
{
    limbo::log::info("DLL shutdown called");
}

extern "C"
#if defined(_WIN32)
__declspec(dllexport)
#endif
GameExports* lm_game_bootstrap(GameMemory*)
{
    exports.update = update;
    exports.shutdown = shutdown;
    return &exports;
}
