#pragma once

// Core
#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/core/Assert.hpp"

// Util
#include "limbo/util/FileIO.hpp"

// Platform
#include "limbo/platform/Platform.hpp"
#include "limbo/platform/Input.hpp"

// Runtime
#include "limbo/runtime/Application.hpp"

// Render - Common
#include "limbo/render/common/RenderContext.hpp"
#include "limbo/render/common/Shader.hpp"
#include "limbo/render/common/Buffer.hpp"
#include "limbo/render/common/Texture.hpp"
#include "limbo/render/common/Camera.hpp"

// Render - 2D
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/2d/Font.hpp"
#include "limbo/render/2d/TextRenderer.hpp"
#include "limbo/render/2d/SpriteMaterial.hpp"

// Render - 3D (skeleton)
#include "limbo/render/3d/Renderer3D.hpp"
#include "limbo/render/3d/Mesh.hpp"
#include "limbo/render/3d/Material.hpp"
#include "limbo/render/3d/Model.hpp"
#include "limbo/render/3d/Lighting.hpp"
#include "limbo/render/3d/Components3D.hpp"

// Assets
#include "limbo/assets/AssetId.hpp"
#include "limbo/assets/Asset.hpp"
#include "limbo/assets/AssetManager.hpp"
#include "limbo/assets/TextureAsset.hpp"
#include "limbo/assets/ShaderAsset.hpp"
#include "limbo/assets/AudioAsset.hpp"

// Scene
#include "limbo/scene/Scene.hpp"
#include "limbo/scene/SceneSerializer.hpp"

// ECS
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/systems/2d/SpriteRenderSystem.hpp"
#include "limbo/ecs/systems/3d/MeshRenderSystem.hpp"

// Debug
#include "limbo/debug/Debug.hpp"

// Physics - 2D
#include "limbo/physics/2d/Physics2D.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/physics/2d/PhysicsSystem2D.hpp"

// Physics - 3D (skeleton)
#include "limbo/physics/3d/Physics3D.hpp"
#include "limbo/physics/3d/PhysicsComponents3D.hpp"
#include "limbo/physics/3d/PhysicsSystem3D.hpp"

// Audio
#include "limbo/audio/AudioEngine.hpp"
#include "limbo/audio/AudioClip.hpp"
#include "limbo/audio/AudioSource.hpp"
#include "limbo/audio/AudioComponents.hpp"
#include "limbo/audio/AudioSystem.hpp"

// Animation
#include "limbo/animation/SpriteSheet.hpp"
#include "limbo/animation/Animation.hpp"
#include "limbo/animation/AnimatorComponent.hpp"
#include "limbo/animation/AnimationSystem.hpp"

// Scripting
#include "limbo/scripting/ScriptEngine.hpp"
#include "limbo/scripting/ScriptComponent.hpp"
#include "limbo/scripting/ScriptSystem.hpp"

// Particles
#include "limbo/particles/ParticleSystem.hpp"
#include "limbo/particles/ParticleComponents.hpp"
#include "limbo/particles/ParticleRenderSystem.hpp"

// Tilemap
#include "limbo/tilemap/Tileset.hpp"
#include "limbo/tilemap/Tilemap.hpp"
#include "limbo/tilemap/TilemapComponent.hpp"
#include "limbo/tilemap/TilemapRenderer.hpp"

// UI
#include "limbo/ui/Widget.hpp"
#include "limbo/ui/Widgets.hpp"
#include "limbo/ui/UICanvas.hpp"

// ImGui
#include "limbo/imgui/ImGuiLayer.hpp"
#include "limbo/imgui/DebugPanels.hpp"
