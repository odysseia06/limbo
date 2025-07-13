#pragma once
#include <string_view>

#ifdef _WIN32
#include <windows.h>
using LibHandle = HMODULE;
#else
#include <dlfcn.h>
using LibHandle = void*;
#endif

struct DynLib
{
    LibHandle handle = nullptr;

    bool load(const char* path);
    void close();

    void* symbol(std::string_view name) const
    {
#ifdef _WIN32
        return handle ? (void*)GetProcAddress(handle, name.data()) : nullptr;
#else
        return handle ? dlsym(handle, name.data()) : nullptr;
#endif
    }
};

