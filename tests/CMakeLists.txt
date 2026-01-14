# Limbo Engine Tests
# Catch2 is already available via FetchContent from Dependencies.cmake

# Collect test sources
set(TEST_SOURCES
    test_main.cpp
    core/test_types.cpp
    core/test_asset_id.cpp
    util/test_file_io.cpp
    math/test_transform.cpp
    ecs/test_world.cpp
    ecs/test_hierarchy.cpp
    scene/test_serialization.cpp
    scene/test_prefab.cpp
    render/test_sprite_atlas.cpp
    assets/test_hot_reload.cpp
    physics/test_physics2d.cpp
    scripting/test_scripting.cpp
)

# Create test executable
add_executable(limbo_tests
    ${TEST_SOURCES}
)

target_link_libraries(limbo_tests
    PRIVATE
        limbo::engine
        Catch2::Catch2
)

# Apply warnings
limbo_set_warnings(limbo_tests)

# Set properties
set_target_properties(limbo_tests PROPERTIES
    FOLDER "Tests"
)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(limbo_tests)
