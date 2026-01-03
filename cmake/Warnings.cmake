# Compiler warnings configuration for Limbo Engine

function(limbo_set_warnings target)
    set(MSVC_WARNINGS
        /W4             # Baseline reasonable warnings
        /w14242         # 'identifier': conversion, possible loss of data
        /w14254         # 'operator': conversion, possible loss of data
        /w14263         # member function does not override any base class virtual member function
        /w14265         # class has virtual functions, but destructor is not virtual
        /w14287         # unsigned/negative constant mismatch
        /we4289         # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
        /w14296         # expression is always 'boolean_value'
        /w14311         # pointer truncation from 'type1' to 'type2'
        /w14545         # expression before comma evaluates to a function which is missing an argument list
        /w14546         # function call before comma missing argument list
        /w14547         # operator before comma has no effect; expected operator with side-effect
        /w14549         # operator before comma has no effect; did you intend 'operator'?
        /w14555         # expression has no effect; expected expression with side-effect
        /w14619         # pragma warning: there is no warning number 'number'
        /w14640         # Enable warning on thread un-safe static member initialization
        /w14826         # Conversion is sign-extended
        /w14905         # wide string literal cast to 'LPSTR'
        /w14906         # string literal cast to 'LPWSTR'
        /w14928         # illegal copy-initialization
        /permissive-    # standards conformance mode
    )

    set(CLANG_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
        -Wno-template-body  # Suppress sol2 template issues in GCC 14+
    )

    if(MSVC)
        set(PROJECT_WARNINGS ${MSVC_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS ${GCC_WARNINGS})
    else()
        message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
    endif()

    # Treat warnings as errors in CI or when explicitly requested
    if(LIMBO_WARNINGS_AS_ERRORS)
        if(MSVC)
            list(APPEND PROJECT_WARNINGS /WX)
        else()
            list(APPEND PROJECT_WARNINGS -Werror)
        endif()
    endif()

    target_compile_options(${target} PRIVATE ${PROJECT_WARNINGS})
endfunction()
