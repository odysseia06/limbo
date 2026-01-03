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
# GLAD2 - OpenGL loader
# ============================================================================
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.6
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  cmake
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
    GIT_TAG        10.2.1
    GIT_SHALLOW    TRUE
)

# ============================================================================
# spdlog - Logging library (uses fmt)
# ============================================================================
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
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
    GIT_TAG        v3.3.0
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
# Official Lua repo has sources in root directory, not src/
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

# GLAD2 - generates OpenGL loader
FetchContent_MakeAvailable(glad)
glad_add_library(glad REPRODUCIBLE API gl:core=4.6)

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
