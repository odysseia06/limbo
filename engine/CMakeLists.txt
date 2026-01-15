# Limbo Engine Library

# Determine library type
if(LIMBO_SHARED_LIB)
    set(LIMBO_LIB_TYPE SHARED)
else()
    set(LIMBO_LIB_TYPE STATIC)
endif()

# Collect all source files from modules
set(LIMBO_ENGINE_SOURCES
    # Core module
    src/core/Types.cpp
    src/core/Time.cpp
    src/core/UUID.cpp
    src/core/FrameAllocator.cpp
    src/core/ThreadPool.cpp
    src/core/MainThreadQueue.cpp

    # Util module
    src/util/FileIO.cpp

    # Platform module
    src/platform/Platform.cpp
    src/platform/Input.cpp

    # Input module
    src/input/InputManager.cpp

    # Runtime module
    src/runtime/Application.cpp

    # Render common module
    src/render/common/api/RenderContext.cpp
    src/render/common/opengl/OpenGLContext.cpp
    src/render/common/Shader.cpp
    src/render/common/Buffer.cpp
    src/render/common/Texture.cpp
    src/render/common/Camera.cpp

    # Graphics module
    src/graphics/Framebuffer.cpp

    # Render 2D module
    src/render/2d/Renderer2D.cpp
    src/render/2d/SpriteAtlas.cpp
    src/render/2d/SpriteAtlasBuilder.cpp
    src/render/2d/Font.cpp
    src/render/2d/TextRenderer.cpp
    src/render/2d/SpriteMaterial.cpp

    # Assets module
    src/assets/AssetId.cpp
    src/assets/AssetImporter.cpp
    src/assets/AssetManager.cpp
    src/assets/AssetLoader.cpp
    src/assets/AssetRegistry.cpp
    src/assets/TextureAsset.cpp
    src/assets/ShaderAsset.cpp
    src/assets/AudioAsset.cpp
    src/assets/SpriteAtlasAsset.cpp
    src/assets/FontAsset.cpp
    src/assets/FileWatcher.cpp
    src/assets/HotReloadManager.cpp

    # Scene module
    src/scene/Scene.cpp
    src/scene/SceneSerializer.cpp
    src/scene/Prefab.cpp
    src/scene/SchemaMigration.cpp

    # ECS module
    src/ecs/World.cpp
    src/ecs/System.cpp
    src/ecs/Hierarchy.cpp
    src/ecs/DeferredDestruction.cpp
    src/ecs/systems/2d/SpriteRenderSystem.cpp
    src/ecs/systems/2d/TextRenderSystem.cpp

    # Debug module
    src/debug/Debug.cpp
    src/debug/GPUTimer.cpp
    src/debug/Log.cpp
    src/debug/Profiler.cpp

    # Physics 2D module
    src/physics/2d/Physics2D.cpp
    src/physics/2d/PhysicsSystem2D.cpp
    src/physics/2d/ContactListener2D.cpp
    src/physics/2d/PhysicsDebug2D.cpp

    # Audio module
    src/audio/AudioEngine.cpp
    src/audio/AudioSystem.cpp

    # Animation module
    src/animation/SpriteSheet.cpp
    src/animation/Animation.cpp
    src/animation/AnimationSystem.cpp

    # Scripting module
    src/scripting/ScriptEngine.cpp
    src/scripting/ScriptSystem.cpp
    src/scripting/ScriptHotReloadManager.cpp

    # Particles module
    src/particles/ParticleSystem.cpp
    src/particles/ParticleRenderSystem.cpp

    # Tilemap module
    src/tilemap/Tileset.cpp
    src/tilemap/Tilemap.cpp
    src/tilemap/TilemapRenderer.cpp

    # UI module
    src/ui/Widget.cpp
    src/ui/Widgets.cpp
    src/ui/UICanvas.cpp

    # ImGui module
    src/imgui/ImGuiLayer.cpp
    src/imgui/DebugPanels.cpp
)

# Collect all header files
set(LIMBO_ENGINE_HEADERS
    # Core headers
    include/limbo/core/Types.hpp
    include/limbo/core/Base.hpp
    include/limbo/core/Assert.hpp
    include/limbo/core/Time.hpp
    include/limbo/core/UUID.hpp
    include/limbo/core/FrameAllocator.hpp
    include/limbo/core/ThreadPool.hpp
    include/limbo/core/MainThreadQueue.hpp

    # Util headers
    include/limbo/util/FileIO.hpp

    # Platform headers
    include/limbo/platform/Platform.hpp
    include/limbo/platform/Input.hpp

    # Input headers
    include/limbo/input/InputManager.hpp

    # Runtime headers
    include/limbo/runtime/Application.hpp

    # Render common headers
    include/limbo/render/common/RenderContext.hpp
    include/limbo/render/common/Shader.hpp
    include/limbo/render/common/Buffer.hpp
    include/limbo/render/common/Texture.hpp
    include/limbo/render/common/Camera.hpp

    # Render 2D headers
    include/limbo/render/2d/Renderer2D.hpp
    include/limbo/render/2d/SpriteAtlas.hpp
    include/limbo/render/2d/SpriteAtlasBuilder.hpp
    include/limbo/render/2d/Font.hpp
    include/limbo/render/2d/TextRenderer.hpp
    include/limbo/render/2d/SpriteMaterial.hpp

    # Render 3D headers (skeleton)
    include/limbo/render/3d/Renderer3D.hpp
    include/limbo/render/3d/Mesh.hpp
    include/limbo/render/3d/Material.hpp
    include/limbo/render/3d/Model.hpp
    include/limbo/render/3d/Lighting.hpp
    include/limbo/render/3d/Components3D.hpp

    # Assets headers
    include/limbo/assets/AssetId.hpp
    include/limbo/assets/AssetImporter.hpp
    include/limbo/assets/Asset.hpp
    include/limbo/assets/AssetManager.hpp
    include/limbo/assets/AssetLoader.hpp
    include/limbo/assets/AssetRegistry.hpp
    include/limbo/assets/TextureAsset.hpp
    include/limbo/assets/ShaderAsset.hpp
    include/limbo/assets/AudioAsset.hpp
    include/limbo/assets/SpriteAtlasAsset.hpp
    include/limbo/assets/FontAsset.hpp
    include/limbo/assets/FileWatcher.hpp
    include/limbo/assets/HotReloadManager.hpp

    # Scene headers
    include/limbo/scene/Scene.hpp
    include/limbo/scene/SceneSerializer.hpp
    include/limbo/scene/Prefab.hpp
    include/limbo/scene/SchemaMigration.hpp

    # ECS headers
    include/limbo/ecs/Components.hpp
    include/limbo/ecs/World.hpp
    include/limbo/ecs/Entity.hpp
    include/limbo/ecs/Hierarchy.hpp
    include/limbo/ecs/DeferredDestruction.hpp
    include/limbo/ecs/System.hpp
    include/limbo/ecs/systems/2d/SpriteRenderSystem.hpp
    include/limbo/ecs/systems/2d/TextRenderSystem.hpp
    include/limbo/ecs/systems/3d/MeshRenderSystem.hpp

    # Debug headers
    include/limbo/debug/Debug.hpp
    include/limbo/debug/Log.hpp
    include/limbo/debug/Validation.hpp
    include/limbo/debug/Profiler.hpp

    # Physics 2D headers
    include/limbo/physics/2d/Physics2D.hpp
    include/limbo/physics/2d/PhysicsComponents2D.hpp
    include/limbo/physics/2d/PhysicsSystem2D.hpp

    # Physics 3D headers (skeleton)
    include/limbo/physics/3d/Physics3D.hpp
    include/limbo/physics/3d/PhysicsComponents3D.hpp
    include/limbo/physics/3d/PhysicsSystem3D.hpp

    # Audio headers
    include/limbo/audio/AudioEngine.hpp
    include/limbo/audio/AudioClip.hpp
    include/limbo/audio/AudioSource.hpp
    include/limbo/audio/AudioComponents.hpp
    include/limbo/audio/AudioSystem.hpp

    # Animation headers
    include/limbo/animation/SpriteSheet.hpp
    include/limbo/animation/Animation.hpp
    include/limbo/animation/AnimatorComponent.hpp
    include/limbo/animation/AnimationSystem.hpp

    # Scripting headers
    include/limbo/scripting/ScriptEngine.hpp
    include/limbo/scripting/ScriptComponent.hpp
    include/limbo/scripting/ScriptSystem.hpp
    include/limbo/scripting/ScriptHotReloadManager.hpp

    # Particles headers
    include/limbo/particles/ParticleSystem.hpp
    include/limbo/particles/ParticleComponents.hpp
    include/limbo/particles/ParticleRenderSystem.hpp

    # Tilemap headers
    include/limbo/tilemap/Tileset.hpp
    include/limbo/tilemap/Tilemap.hpp
    include/limbo/tilemap/TilemapComponent.hpp
    include/limbo/tilemap/TilemapRenderer.hpp

    # UI headers
    include/limbo/ui/Widget.hpp
    include/limbo/ui/Widgets.hpp
    include/limbo/ui/UICanvas.hpp

    # ImGui headers
    include/limbo/imgui/ImGuiLayer.hpp
    include/limbo/imgui/DebugPanels.hpp

    # Main engine header
    include/limbo/Limbo.hpp
)

# Create engine library
add_library(limbo_engine ${LIMBO_LIB_TYPE}
    ${LIMBO_ENGINE_SOURCES}
    ${LIMBO_ENGINE_HEADERS}
)

# Add alias for consistent usage
add_library(limbo::engine ALIAS limbo_engine)

# Include directories
target_include_directories(limbo_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(limbo_engine
    PUBLIC
        glfw
        glm::glm
        EnTT::EnTT
        fmt::fmt
        spdlog::spdlog
        imgui
        nlohmann_json::nlohmann_json
        box2d
        lua::lua
        sol2
    PRIVATE
        glad
        stb
        miniaudio
)

# Compile definitions
target_compile_definitions(limbo_engine
    PUBLIC
        $<$<CONFIG:Debug>:LIMBO_DEBUG=1>
        $<$<CONFIG:Release>:LIMBO_RELEASE=1>
        $<$<CONFIG:RelWithDebInfo>:LIMBO_RELEASE=1>
        $<$<BOOL:${LIMBO_ENABLE_GPU_DEBUG}>:LIMBO_GPU_DEBUG=1>
    PRIVATE
        $<$<BOOL:${LIMBO_SHARED_LIB}>:LIMBO_EXPORT>
)

# Apply warnings
limbo_set_warnings(limbo_engine)

# Set output name
set_target_properties(limbo_engine PROPERTIES
    OUTPUT_NAME "limbo"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# IDE folder organization
set_target_properties(limbo_engine PROPERTIES FOLDER "Engine")
