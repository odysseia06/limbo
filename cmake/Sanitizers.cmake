# Sanitizers configuration for Limbo Engine
# Supports Address Sanitizer (ASan), Undefined Behavior Sanitizer (UBSan), and Thread Sanitizer (TSan)

# Function to apply sanitizer flags to a target
function(limbo_enable_sanitizers target)
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        message(WARNING "Sanitizers are only supported with Clang and GCC. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    set(SANITIZER_FLAGS "")
    set(SANITIZER_LINK_FLAGS "")

    # Address Sanitizer
    if(LIMBO_ENABLE_ASAN)
        list(APPEND SANITIZER_FLAGS -fsanitize=address -fno-omit-frame-pointer)
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=address)
        message(STATUS "Address Sanitizer enabled for target: ${target}")
    endif()

    # Undefined Behavior Sanitizer
    if(LIMBO_ENABLE_UBSAN)
        list(APPEND SANITIZER_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=undefined)
        message(STATUS "Undefined Behavior Sanitizer enabled for target: ${target}")
    endif()

    # Thread Sanitizer (mutually exclusive with ASan)
    if(LIMBO_ENABLE_TSAN)
        if(LIMBO_ENABLE_ASAN)
            message(FATAL_ERROR "Thread Sanitizer (TSan) cannot be used together with Address Sanitizer (ASan)")
        endif()
        list(APPEND SANITIZER_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=thread)
        message(STATUS "Thread Sanitizer enabled for target: ${target}")
    endif()

    # Apply flags if any sanitizer is enabled
    if(SANITIZER_FLAGS)
        target_compile_options(${target} PRIVATE ${SANITIZER_FLAGS})
        target_link_options(${target} PRIVATE ${SANITIZER_LINK_FLAGS})
    endif()
endfunction()

# Global sanitizer application (for use in Options.cmake compatibility)
function(limbo_apply_global_sanitizers)
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        if(LIMBO_ENABLE_ASAN OR LIMBO_ENABLE_UBSAN OR LIMBO_ENABLE_TSAN)
            message(WARNING "Sanitizers are only supported with Clang and GCC. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
        endif()
        return()
    endif()

    # Address Sanitizer
    if(LIMBO_ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
        message(STATUS "Address Sanitizer enabled globally")
    endif()

    # Undefined Behavior Sanitizer
    if(LIMBO_ENABLE_UBSAN)
        add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined)
        message(STATUS "Undefined Behavior Sanitizer enabled globally")
    endif()

    # Thread Sanitizer
    if(LIMBO_ENABLE_TSAN)
        if(LIMBO_ENABLE_ASAN)
            message(FATAL_ERROR "Thread Sanitizer (TSan) cannot be used together with Address Sanitizer (ASan)")
        endif()
        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread)
        message(STATUS "Thread Sanitizer enabled globally")
    endif()
endfunction()
