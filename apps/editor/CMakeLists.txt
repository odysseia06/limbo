# Limbo Editor Application

add_executable(limbo_editor
    main.cpp
    EditorApp.cpp
    EditorApp.hpp
    panels/SceneHierarchyPanel.cpp
    panels/SceneHierarchyPanel.hpp
    panels/InspectorPanel.cpp
    panels/InspectorPanel.hpp
    panels/ViewportPanel.cpp
    panels/ViewportPanel.hpp
    panels/AssetBrowserPanel.cpp
    panels/AssetBrowserPanel.hpp
    panels/AssetPipelinePanel.cpp
    panels/AssetPipelinePanel.hpp
)

target_link_libraries(limbo_editor
    PRIVATE
        limbo::engine
        imgui
)

target_include_directories(limbo_editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Apply warnings
limbo_set_warnings(limbo_editor)

# Set output directory
set_target_properties(limbo_editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER "Apps"
)
