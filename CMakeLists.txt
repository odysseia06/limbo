cmake_minimum_required(VERSION 3.25)

# Project definition
project(limbo
    VERSION 0.1.0
    DESCRIPTION "Limbo Game Engine"
    LANGUAGES C CXX
)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE/tooling support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Warnings)
include(Options)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Fetch all dependencies automatically (no vcpkg needed!)
include(Dependencies)

# Engine library
add_subdirectory(engine)

# Applications
if(LIMBO_BUILD_SANDBOX)
    add_subdirectory(apps/sandbox)
endif()

if(LIMBO_BUILD_TOOLS)
    add_subdirectory(apps/editor)
endif()

# Tools
if(LIMBO_BUILD_TOOLS)
    add_subdirectory(tools/assetcooker)
endif()

# Tests
if(LIMBO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
