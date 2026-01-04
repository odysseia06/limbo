# Dependencies.cmake - Fetch all dependencies via FetchContent

include(FetchContent)

# Speed up FetchContent by not updating on every configure
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

message(STATUS "Fetching dependencies...")

# ============================================================================
# GLFW - Window and input handling
# ============================================================================
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)

# ============================================================================
# GLM - Math library
# ============================================================================
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

# ============================================================================
# EnTT - Entity Component System
# ============================================================================
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2
    GIT_SHALLOW    TRUE
)

# ============================================================================
# fmt - Formatting library
# ============================================================================
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.1.4
    GIT_SHALLOW    TRUE
)

# ============================================================================
# spdlog - Logging library (uses fmt)
# ============================================================================
# Use std::format instead of fmt to avoid compatibility issues with Clang 21
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.0
    GIT_SHALLOW    TRUE
)

# ============================================================================
# Dear ImGui - Immediate mode GUI
# ============================================================================
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)

# ============================================================================
# nlohmann/json - JSON library for serialization
# ============================================================================
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# ============================================================================
# miniaudio - Single-header audio library
# ============================================================================
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE
)

# ============================================================================
# Lua - Scripting language (from GitHub mirror)
# ============================================================================
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.6
    GIT_SHALLOW    TRUE
)

# ============================================================================
# sol2 - C++ Lua binding library
# ============================================================================
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.5.0
    GIT_SHALLOW    TRUE
)

# ============================================================================
# Box2D - 2D Physics engine
# ============================================================================
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_USER_SETTINGS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG        v2.4.2
    GIT_SHALLOW    TRUE
)

# ============================================================================
# Catch2 - Testing framework (only if tests enabled)
# ============================================================================
if(LIMBO_BUILD_TESTS)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
        GIT_SHALLOW    TRUE
    )
endif()

# ============================================================================
# stb - Header-only image/utility libraries
# ============================================================================
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ============================================================================
# Make dependencies available
# ============================================================================

# Fetch all at once for efficiency
FetchContent_MakeAvailable(glfw glm entt fmt spdlog stb nlohmann_json box2d)

# Lua needs manual build (no CMakeLists.txt in official source)
FetchContent_MakeAvailable(lua)
FetchContent_GetProperties(lua)

# Create Lua library from source files
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "onelua\\.c$")

add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC "${lua_SOURCE_DIR}")
target_compile_definitions(lua_static PUBLIC LUA_USE_LONGJMP=1)
if(UNIX)
    target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
    target_link_libraries(lua_static PRIVATE m dl)
endif()
add_library(lua::lua ALIAS lua_static)

# sol2
FetchContent_MakeAvailable(sol2)
FetchContent_GetProperties(sol2)

# Patch sol2 for Clang compatibility
# The issue is that sol2's call functions have noexcept specifiers, but lua_CFunction
# is int(*)(lua_State*) without noexcept. In C++17+, noexcept is part of the type.
# Fix by treating Clang the same as MSVC (which already has this workaround).
set(SOL2_STATELESS_HPP "${sol2_SOURCE_DIR}/include/sol/function_types_stateless.hpp")
if(EXISTS "${SOL2_STATELESS_HPP}")
    file(READ "${SOL2_STATELESS_HPP}" SOL2_CONTENT)
    # Change the condition to also exclude Clang from noexcept specifier
    string(REPLACE
        "#if SOL_IS_ON(SOL_COMPILER_VCXX)"
        "#if SOL_IS_ON(SOL_COMPILER_VCXX) || SOL_IS_ON(SOL_COMPILER_CLANG)"
        SOL2_CONTENT "${SOL2_CONTENT}")
    file(WRITE "${SOL2_STATELESS_HPP}" "${SOL2_CONTENT}")
    message(STATUS "Patched sol2 for Clang noexcept compatibility")
endif()

# GLAD - use pre-generated sources from extern/glad
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/extern/glad/src/gl.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/extern/glad/include
)

# ImGui needs manual setup (no CMakeLists.txt in repo)
FetchContent_MakeAvailable(imgui)

# Create ImGui library with GLFW + OpenGL3 backends
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw)

# On Linux, ImGui GLFW backend needs X11 for certain operations
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(imgui PUBLIC ${X11_LIBRARIES})
endif()

# Catch2 (only if tests enabled)
if(LIMBO_BUILD_TESTS)
    FetchContent_MakeAvailable(catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# stb include directory
FetchContent_GetProperties(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# miniaudio include directory
FetchContent_MakeAvailable(miniaudio)
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})

message(STATUS "All dependencies fetched successfully!")
