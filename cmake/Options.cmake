# Build options for Limbo Engine

# Build configuration options
option(LIMBO_BUILD_TESTS "Build unit tests" ON)
option(LIMBO_BUILD_TOOLS "Build asset tools" ON)
option(LIMBO_BUILD_SANDBOX "Build sandbox application" ON)
option(LIMBO_BUILD_EDITOR "Build editor application" OFF)

# Development options
option(LIMBO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(LIMBO_ENABLE_ASAN "Enable Address Sanitizer (GCC/Clang)" OFF)

# Engine configuration
option(LIMBO_SHARED_LIB "Build limbo_engine as shared library" OFF)

# Debug options
option(LIMBO_ENABLE_PROFILING "Enable profiling instrumentation" OFF)
option(LIMBO_ENABLE_GPU_DEBUG "Enable GPU debug markers and validation" ON)

# Apply sanitizers if enabled
if(LIMBO_ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    else()
        message(WARNING "Address Sanitizer is only supported with Clang and GCC")
    endif()
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=== Limbo Engine Configuration ===")
message(STATUS "  Build Type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests:         ${LIMBO_BUILD_TESTS}")
message(STATUS "  Build Tools:         ${LIMBO_BUILD_TOOLS}")
message(STATUS "  Shared Library:      ${LIMBO_SHARED_LIB}")
message(STATUS "  Warnings as Errors:  ${LIMBO_WARNINGS_AS_ERRORS}")
message(STATUS "  Address Sanitizer:   ${LIMBO_ENABLE_ASAN}")
message(STATUS "  GPU Debug:           ${LIMBO_ENABLE_GPU_DEBUG}")
message(STATUS "==================================")
message(STATUS "")
