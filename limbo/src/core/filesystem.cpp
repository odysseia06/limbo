#include "limbo/core/filesystem.hpp"
#include <cstdlib>          // std::getenv
#if defined(_WIN32)
#include <windows.h>
#include <vector>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>     // readlink
#include <limits.h>
#endif
// TODO: I copy-pasted this and i will figure out what is going on here later
namespace limbo::fs {
    namespace fs = std::filesystem;

    //------------------------------------------------------------
    // helpers ----------------------------------------------------
    static fs::path deduce_exe_path()
    {
#if defined(_WIN32)
        std::vector<wchar_t> buffer(MAX_PATH);
        DWORD len = 0;
        while (true) {
            len = GetModuleFileNameW(nullptr, buffer.data(),
                static_cast<DWORD>(buffer.size()));
            if (len == 0)
                return {};
            if (len < buffer.size() - 1)
                break;                          // success
            buffer.resize(buffer.size() * 2);   // grow and retry
        }
        return fs::path(buffer.data(), buffer.data() + len);
#elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);   // get required size
        std::string buf(size, '\0');
        if (_NSGetExecutablePath(buf.data(), &size) == 0)
            return fs::canonical(buf);
        return {};
#else                                       // Linux / BSD
        char buf[PATH_MAX + 1];
        ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX);
        if (len != -1)
            return fs::canonical(std::string_view(buf, len));
        return {};
#endif
    }

    //------------------------------------------------------------
    // public API -------------------------------------------------
    fs::path executable_path()
    {
        static fs::path cached = deduce_exe_path();
        return cached;
    }

    fs::path logs_directory()
    {
        static fs::path dir = [] {
            fs::path p = executable_path().empty()
                ? fs::current_path()
                : executable_path().parent_path();
            p /= "logs";
            fs::create_directories(p);
            return p;
            }();
        return dir;
    }

} // namespace limbo::fs
