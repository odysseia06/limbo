#pragma once
#include <filesystem>
#include <chrono>
#include "dylib.hpp"
#include "public/game_api.hpp"
#include <limbo/core/log.hpp>
#include <limbo/core/filesystem.hpp>

class GameHost
{
public:
    bool load(const char* stem);
    void unload();
    void tick(double dt);
    void maybe_hot_reload();

private:
    DynLib  lib_;
    GameExports* exports_ = nullptr;
    GameMemory mem_{};

    std::filesystem::path dllPath_, tmpPath_;
    std::filesystem::file_time_type lastWrite_;
};
