#include "limbo/core/log.hpp"
#include "limbo/core/clock.hpp"
#include "limbo/core/filesystem.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>


#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace limbo::log {

    static constexpr std::string_view level_str(Level lv) noexcept
    {
        switch (lv) {
        case Level::Trace:    return "TRACE";
        case Level::Debug:    return "DEBUG";
        case Level::Info:     return "INFO ";
        case Level::Warning:  return "WARN ";
        case Level::Error:    return "ERROR";
        case Level::Fatal:    return "FATAL";
        }
        return "???? ";
    }

#ifdef _WIN32
    static WORD win_color(Level lv) noexcept
    {
        switch (lv) {
        case Level::Trace:    return 8;    // grey
        case Level::Debug:    return 11;   // cyan
        case Level::Info:     return 10;   // green
        case Level::Warning:  return 14;   // yellow
        case Level::Error:    return 12;   // red
        case Level::Fatal:    return 79;   // bright red on white
        }
        return 7;
    }
#else
    static constexpr std::string_view ansi_color(Level lv) noexcept
    {
        switch (lv) {
        case Level::Trace:    return "\x1b[37m";    // grey
        case Level::Debug:    return "\x1b[36m";    // cyan
        case Level::Info:     return "\x1b[32m";    // green
        case Level::Warning:  return "\x1b[33m";    // yellow
        case Level::Error:    return "\x1b[31m";    // red
        case Level::Fatal:    return "\x1b[41;97m"; // red bg, white fg
        }
        return "\x1b[0m";
    }
#endif

    struct Sink {
        explicit Sink(Level min) : minLevel(min) {}
        virtual ~Sink() = default;
        virtual void write(Level lv, std::string_view line) = 0;
        Level minLevel;
    };

    class ConsoleSink : public Sink {
    public:
        using Sink::Sink;

        void write(Level lv, std::string_view line) override
        {
#ifdef _WIN32
            static HANDLE hConsole = [] {
                HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
                DWORD mode{};
                GetConsoleMode(h, &mode);
                mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(h, mode);
                return h;
                }();
            CONSOLE_SCREEN_BUFFER_INFO oldInfo{};
            GetConsoleScreenBufferInfo(hConsole, &oldInfo);
            SetConsoleTextAttribute(hConsole, win_color(lv));
            std::cout << line << '\n';
            SetConsoleTextAttribute(hConsole, oldInfo.wAttributes);
#else
            std::cout << ansi_color(lv) << line << "\x1b[0m\n";
#endif
        }
    };

    class FileSink : public Sink {
    public:
        using Sink::Sink;

        explicit FileSink(Level min) : Sink(min)
        {
            namespace fs = std::filesystem;
            fs::create_directories(fs::path(limbo::fs::logs_directory()));

            auto ts = Clock::iso_datetime();       // "2025-07-12 21:56:04"
            for (char& c : ts) if (c == ':' || c == ' ') c = '_';

            auto filename = limbo::fs::logs_directory() / (ts + ".log");
            file_.open(filename, std::ios::out | std::ios::app);
        }

        void write(Level /*lv*/, std::string_view line) override
        {
            if (file_.is_open()) {
                file_ << line << '\n';
                file_.flush();
            }
        }
    private:
        std::ofstream file_;
    };

    static std::mutex               g_mutex;
    static std::array<std::unique_ptr<Sink>, 2> g_sinks; // [0]=console, [1]=file

    //--------------------------------------------------
    // public API
    //--------------------------------------------------
    void init(Level consoleMin, Level fileMin)
    {
        std::lock_guard lock(g_mutex);
        g_sinks[0] = std::make_unique<ConsoleSink>(consoleMin);
        g_sinks[1] = std::make_unique<FileSink>(fileMin);
    }

    void shutdown()
    {
        std::lock_guard lock(g_mutex);
        for (auto& s : g_sinks)
            s.reset();
    }

    void message(Level lv,
        std::string_view fmt,
        std::format_args args)
    {
        std::string text = std::vformat(fmt, args);

        // prepend timestamp + level
        std::string line = std::format("[{:%F %T}] [{:<5}] {}",
            std::chrono::system_clock::now(), level_str(lv), text);

        std::lock_guard lock(g_mutex);
        for (auto& s : g_sinks) {
            if (s && lv >= s->minLevel)
                s->write(lv, line);
        }
    }
}
