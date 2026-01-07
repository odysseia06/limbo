#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo::validation {

// =============================================================================
// Validation Configuration
// =============================================================================

// Validation is enabled in Debug builds by default
#ifdef LIMBO_DEBUG
#define LIMBO_VALIDATION_ENABLED 1
#else
#define LIMBO_VALIDATION_ENABLED 0
#endif

// =============================================================================
// Renderer Validation
// =============================================================================

#if LIMBO_VALIDATION_ENABLED

class RendererValidation {
public:
    static void beginScene() {
        if (s_sceneActive) {
            LIMBO_LOG_RENDER_ERROR("beginScene() called while a scene is already active. "
                                   "Did you forget to call endScene()?");
        }
        s_sceneActive = true;
    }

    static void endScene() {
        if (!s_sceneActive) {
            LIMBO_LOG_RENDER_ERROR("endScene() called without a matching beginScene()");
        }
        s_sceneActive = false;
    }

    static void draw() {
        if (!s_sceneActive) {
            LIMBO_LOG_RENDER_ERROR("Draw call made outside of beginScene()/endScene() block");
        }
    }

    static void reset() { s_sceneActive = false; }

    [[nodiscard]] static bool isSceneActive() { return s_sceneActive; }

private:
    static inline bool s_sceneActive = false;
};

#define LIMBO_VALIDATE_RENDERER_BEGIN_SCENE() ::limbo::validation::RendererValidation::beginScene()
#define LIMBO_VALIDATE_RENDERER_END_SCENE()   ::limbo::validation::RendererValidation::endScene()
#define LIMBO_VALIDATE_RENDERER_DRAW()        ::limbo::validation::RendererValidation::draw()
#define LIMBO_VALIDATE_RENDERER_RESET()       ::limbo::validation::RendererValidation::reset()

#else

#define LIMBO_VALIDATE_RENDERER_BEGIN_SCENE() ((void)0)
#define LIMBO_VALIDATE_RENDERER_END_SCENE()   ((void)0)
#define LIMBO_VALIDATE_RENDERER_DRAW()        ((void)0)
#define LIMBO_VALIDATE_RENDERER_RESET()       ((void)0)

#endif

// =============================================================================
// ECS Validation
// =============================================================================

#if LIMBO_VALIDATION_ENABLED

class ECSValidation {
public:
    template<typename Registry>
    static bool validateEntity(Registry& registry, auto entity, const char* operation) {
        if (!registry.valid(entity)) {
            LIMBO_LOG_ECS_ERROR("{}: Invalid entity handle", operation);
            return false;
        }
        return true;
    }

    template<typename Registry, typename Component>
    static bool validateHasComponent(Registry& registry, auto entity, const char* operation) {
        if (!registry.valid(entity)) {
            LIMBO_LOG_ECS_ERROR("{}: Invalid entity handle", operation);
            return false;
        }
        if (!registry.template all_of<Component>(entity)) {
            LIMBO_LOG_ECS_ERROR("{}: Entity does not have the requested component", operation);
            return false;
        }
        return true;
    }
};

#define LIMBO_VALIDATE_ENTITY(registry, entity, operation)                                         \
    ::limbo::validation::ECSValidation::validateEntity(registry, entity, operation)

#define LIMBO_VALIDATE_HAS_COMPONENT(registry, entity, Component, operation)                       \
    ::limbo::validation::ECSValidation::validateHasComponent<decltype(registry), Component>(       \
        registry, entity, operation)

#else

#define LIMBO_VALIDATE_ENTITY(registry, entity, operation)                     true
#define LIMBO_VALIDATE_HAS_COMPONENT(registry, entity, Component, operation)   true

#endif

// =============================================================================
// Asset Validation
// =============================================================================

#if LIMBO_VALIDATION_ENABLED

class AssetValidation {
public:
    template<typename T>
    static bool validateHandle(const T& handle, const char* operation) {
        if (!handle) {
            LIMBO_LOG_ASSET_ERROR("{}: Invalid or null asset handle", operation);
            return false;
        }
        return true;
    }

    static bool validatePath(const char* path, const char* operation) {
        if (path == nullptr || path[0] == '\0') {
            LIMBO_LOG_ASSET_ERROR("{}: Empty or null asset path", operation);
            return false;
        }
        return true;
    }
};

#define LIMBO_VALIDATE_ASSET_HANDLE(handle, operation)                                             \
    ::limbo::validation::AssetValidation::validateHandle(handle, operation)

#define LIMBO_VALIDATE_ASSET_PATH(path, operation)                                                 \
    ::limbo::validation::AssetValidation::validatePath(path, operation)

#else

#define LIMBO_VALIDATE_ASSET_HANDLE(handle, operation) true
#define LIMBO_VALIDATE_ASSET_PATH(path, operation)     true

#endif

// =============================================================================
// Physics Validation
// =============================================================================

#if LIMBO_VALIDATION_ENABLED

class PhysicsValidation {
public:
    static void beginStep() {
        if (s_stepping) {
            LIMBO_LOG_PHYSICS_ERROR("Physics step started while already stepping. "
                                    "Nested physics steps are not allowed.");
        }
        s_stepping = true;
    }

    static void endStep() {
        if (!s_stepping) {
            LIMBO_LOG_PHYSICS_ERROR("Physics step ended without a matching begin.");
        }
        s_stepping = false;
    }

    [[nodiscard]] static bool isStepping() { return s_stepping; }

    static void reset() { s_stepping = false; }

private:
    static inline bool s_stepping = false;
};

#define LIMBO_VALIDATE_PHYSICS_BEGIN_STEP() ::limbo::validation::PhysicsValidation::beginStep()
#define LIMBO_VALIDATE_PHYSICS_END_STEP()   ::limbo::validation::PhysicsValidation::endStep()
#define LIMBO_VALIDATE_PHYSICS_RESET()      ::limbo::validation::PhysicsValidation::reset()

#else

#define LIMBO_VALIDATE_PHYSICS_BEGIN_STEP() ((void)0)
#define LIMBO_VALIDATE_PHYSICS_END_STEP()   ((void)0)
#define LIMBO_VALIDATE_PHYSICS_RESET()      ((void)0)

#endif

// =============================================================================
// General Validation Utilities
// =============================================================================

#if LIMBO_VALIDATION_ENABLED

#define LIMBO_VALIDATE(condition, category, message)                                               \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            LIMBO_LOG_##category##_ERROR("Validation failed: {}", message);                        \
        }                                                                                          \
    } while (false)

#define LIMBO_VALIDATE_NOT_NULL(ptr, category, name)                                               \
    do {                                                                                           \
        if ((ptr) == nullptr) {                                                                    \
            LIMBO_LOG_##category##_ERROR("{} is null", name);                                      \
        }                                                                                          \
    } while (false)

#else

#define LIMBO_VALIDATE(condition, category, message)   ((void)0)
#define LIMBO_VALIDATE_NOT_NULL(ptr, category, name)   ((void)0)

#endif

}  // namespace limbo::validation
