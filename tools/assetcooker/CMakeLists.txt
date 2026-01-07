# Asset Cooker Tool

if(NOT LIMBO_BUILD_TOOLS)
    return()
endif()

add_executable(assetcooker
    main.cpp
)

target_link_libraries(assetcooker
    PRIVATE
        limbo::engine
        fmt::fmt
        spdlog::spdlog
)

# Apply warnings
limbo_set_warnings(assetcooker)

# Set properties
set_target_properties(assetcooker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER "Tools"
)
