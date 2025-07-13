#include "dylib.hpp"

bool DynLib::load(const char* path)
{
#ifdef _WIN32
    handle = LoadLibraryA(path);
#else
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
    return handle != nullptr;
}

void DynLib::close()
{
    if (!handle) return;
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
    handle = nullptr;
}
