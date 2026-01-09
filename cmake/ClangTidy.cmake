# ClangTidy configuration for Limbo Engine
# Provides a target for running clang-tidy on the codebase

option(LIMBO_ENABLE_CLANG_TIDY "Enable clang-tidy during build" OFF)

# Find clang-tidy executable
find_program(CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15
    DOC "Path to clang-tidy executable"
)

# Function to enable clang-tidy on a target during build
function(limbo_target_enable_clang_tidy target)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(WARNING "clang-tidy not found, skipping for target: ${target}")
        return()
    endif()

    set_target_properties(${target} PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
    )
    message(STATUS "clang-tidy enabled for target: ${target}")
endfunction()

# Function to create a clang-tidy check target
function(limbo_add_clang_tidy_target)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(STATUS "clang-tidy not found, clang-tidy-check target will not be created")
        return()
    endif()

    # Collect all source files from engine
    file(GLOB_RECURSE ENGINE_SOURCES
        "${CMAKE_SOURCE_DIR}/engine/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/engine/include/*.hpp"
    )

    # Collect all source files from apps
    file(GLOB_RECURSE APP_SOURCES
        "${CMAKE_SOURCE_DIR}/apps/*.cpp"
        "${CMAKE_SOURCE_DIR}/apps/*.hpp"
    )

    set(ALL_SOURCES ${ENGINE_SOURCES} ${APP_SOURCES})

    # Create the clang-tidy check target
    add_custom_target(clang-tidy-check
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            -p ${CMAKE_BINARY_DIR}
            ${ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on all source files..."
        VERBATIM
    )

    message(STATUS "Created clang-tidy-check target")
endfunction()

# Function to create a clang-tidy fix target (applies fixes automatically)
function(limbo_add_clang_tidy_fix_target)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(STATUS "clang-tidy not found, clang-tidy-fix target will not be created")
        return()
    endif()

    # Collect all source files from engine
    file(GLOB_RECURSE ENGINE_SOURCES
        "${CMAKE_SOURCE_DIR}/engine/src/*.cpp"
    )

    # Collect all source files from apps
    file(GLOB_RECURSE APP_SOURCES
        "${CMAKE_SOURCE_DIR}/apps/*.cpp"
    )

    set(ALL_SOURCES ${ENGINE_SOURCES} ${APP_SOURCES})

    # Create the clang-tidy fix target
    add_custom_target(clang-tidy-fix
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            --fix
            --fix-errors
            -p ${CMAKE_BINARY_DIR}
            ${ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy with auto-fix on all source files..."
        VERBATIM
    )

    message(STATUS "Created clang-tidy-fix target")
endfunction()

# Apply clang-tidy to build if enabled
if(LIMBO_ENABLE_CLANG_TIDY)
    if(CLANG_TIDY_EXECUTABLE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
        message(STATUS "clang-tidy enabled globally during build")
    else()
        message(WARNING "LIMBO_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
endif()
