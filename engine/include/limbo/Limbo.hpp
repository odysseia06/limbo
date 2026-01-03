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

// Render
#include "limbo/render/RenderContext.hpp"
#include "limbo/render/Shader.hpp"
#include "limbo/render/Buffer.hpp"
#include "limbo/render/Texture.hpp"
#include "limbo/render/Camera.hpp"
#include "limbo/render/Renderer2D.hpp"

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
#include "limbo/ecs/systems/SpriteRenderSystem.hpp"

// Debug
#include "limbo/debug/Debug.hpp"

// Physics
#include "limbo/physics/Physics2D.hpp"
#include "limbo/physics/PhysicsComponents.hpp"
#include "limbo/physics/PhysicsSystem.hpp"

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
