#pragma once

#include "limbo/ecs/World.hpp"

#include <unordered_set>
#include <vector>

namespace limbo {

/**
 * DeferredDestruction - Safe entity destruction during physics callbacks
 *
 * When scripts destroy entities during collision/trigger callbacks, Box2D may
 * crash because it's still iterating contacts. This system queues destruction
 * requests and processes them after the physics step is complete.
 *
 * Usage:
 *   // During physics callback (safe):
 *   DeferredDestruction::queueDestroy(world, entity);
 *
 *   // After physics dispatch (in PhysicsSystem2D::update):
 *   DeferredDestruction::flush(world);
 *
 * The system tracks whether we're currently in a physics callback context.
 * When inside physics, destroy() queues the entity. When outside, it destroys
 * immediately for normal gameplay code.
 */
class DeferredDestruction {
public:
    /**
     * Queue entity for deferred destruction
     * If called during physics callbacks, queues for later.
     * If called outside physics, destroys immediately.
     */
    static void destroy(World& world, World::EntityId entity);

    /**
     * Queue entity for destruction (always deferred, regardless of context)
     */
    static void queueDestroy(World& world, World::EntityId entity);

    /**
     * Process all queued destructions
     * Call this after physics dispatch is complete.
     */
    static void flush(World& world);

    /**
     * Check if entity is queued for destruction
     */
    [[nodiscard]] static bool isPendingDestruction(World::EntityId entity);

    /**
     * Clear pending destructions without processing them
     * Useful for scene changes or error recovery.
     */
    static void clear();

    /**
     * Begin physics callback context
     * While in this context, destroy() will queue instead of immediate destroy.
     */
    static void beginPhysicsContext();

    /**
     * End physics callback context
     */
    static void endPhysicsContext();

    /**
     * Check if we're currently in physics callback context
     */
    [[nodiscard]] static bool isInPhysicsContext();

    /**
     * RAII helper for physics context
     */
    class ScopedPhysicsContext {
    public:
        ScopedPhysicsContext() { DeferredDestruction::beginPhysicsContext(); }
        ~ScopedPhysicsContext() { DeferredDestruction::endPhysicsContext(); }

        ScopedPhysicsContext(const ScopedPhysicsContext&) = delete;
        ScopedPhysicsContext& operator=(const ScopedPhysicsContext&) = delete;
    };

private:
    static std::vector<std::pair<World*, World::EntityId>> s_pendingDestruction;
    static std::unordered_set<u32> s_pendingEntityIds;  // For fast lookup
    static i32 s_physicsContextDepth;
};

}  // namespace limbo
