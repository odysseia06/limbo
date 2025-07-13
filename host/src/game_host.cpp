#include "game_host.hpp"

#ifdef _WIN32
static constexpr const char* EXT = ".dll";
#else
static constexpr const char* EXT = ".so";
#endif

// helper --------------------------------------------------------------
static std::filesystem::path copy_to_temp(const std::filesystem::path& src)
{
    auto dst = src;
    dst += ".tmp";
    std::filesystem::copy_file(src, dst,
        std::filesystem::copy_options::overwrite_existing);
    return dst;
}

// public --------------------------------------------------------------
bool GameHost::load(const char* stem)
{
    dllPath_ = limbo::fs::executable_path().parent_path()
        / (std::string(stem) + EXT);
    tmpPath_ = copy_to_temp(dllPath_);

    if (!lib_.load(tmpPath_.string().c_str()))
    {
        limbo::log::error("Failed to load {}", dllPath_.string());
        return false;
    }

    auto* bootstrap = reinterpret_cast<GameBootstrapFn>(
        lib_.symbol("lm_game_bootstrap"));

    if (!bootstrap)
    {
        limbo::log::error("lm_game_bootstrap missing in {}", dllPath_.string());
        return false;
    }

    exports_ = bootstrap(&mem_);
    if (!exports_ || !exports_->update)
    {
        limbo::log::error("GameExports invalid");
        return false;
    }

    lastWrite_ = std::filesystem::last_write_time(dllPath_);
    limbo::log::info("Loaded {}", dllPath_.filename().string());
    return true;
}

void GameHost::unload()
{
    if (exports_ && exports_->shutdown)
        exports_->shutdown(&mem_);

    lib_.close();
    exports_ = nullptr;
    std::error_code ec;
    std::filesystem::remove(tmpPath_, ec);
}

void GameHost::tick(double dt)
{
    if (exports_) exports_->update(&mem_, dt);
}

void GameHost::maybe_hot_reload()
{
    auto nowWrite = std::filesystem::last_write_time(dllPath_);
    if (nowWrite != lastWrite_)
    {
        limbo::log::info("Change detected – reloading DLL...");
        unload();
        load(dllPath_.stem().string().c_str());
    }
}
