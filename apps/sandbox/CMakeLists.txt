# Sandbox Application

add_executable(sandbox
    main.cpp
)

target_link_libraries(sandbox
    PRIVATE
        limbo::engine
        imgui
)

# Apply warnings
limbo_set_warnings(sandbox)

# Set output directory
set_target_properties(sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER "Apps"
)

# Copy assets to output directory (when we have assets)
# add_custom_command(TARGET sandbox POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${CMAKE_SOURCE_DIR}/assets
#         $<TARGET_FILE_DIR:sandbox>/assets
# )
