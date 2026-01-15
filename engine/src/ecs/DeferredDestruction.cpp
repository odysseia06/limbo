#include "limbo/ecs/DeferredDestruction.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

// Static member definitions
std::vector<std::pair<World*, World::EntityId>> DeferredDestruction::s_pendingDestruction;
std::unordered_set<u32> DeferredDestruction::s_pendingEntityIds;
i32 DeferredDestruction::s_physicsContextDepth = 0;

void DeferredDestruction::destroy(World& world, World::EntityId entity) {
    if (s_physicsContextDepth > 0) {
        // We're in physics context - queue for later
        queueDestroy(world, entity);
    } else {
        // Outside physics context - destroy immediately
        if (world.isValid(entity)) {
            world.destroyEntity(entity);
        }
    }
}

void DeferredDestruction::queueDestroy(World& world, World::EntityId entity) {
    // Check if already queued
    u32 rawId = static_cast<u32>(entity);
    if (s_pendingEntityIds.find(rawId) != s_pendingEntityIds.end()) {
        return;  // Already queued
    }

    // Check if entity is valid
    if (!world.isValid(entity)) {
        LIMBO_LOG_ECS_WARN("Attempted to queue invalid entity for destruction");
        return;
    }

    s_pendingDestruction.emplace_back(&world, entity);
    s_pendingEntityIds.insert(rawId);

    LIMBO_LOG_ECS_DEBUG("Queued entity {} for deferred destruction", rawId);
}

void DeferredDestruction::flush(World& world) {
    if (s_pendingDestruction.empty()) {
        return;
    }

    // Copy and clear before processing to handle nested destruction
    auto pending = std::move(s_pendingDestruction);
    s_pendingDestruction.clear();
    s_pendingEntityIds.clear();

    i32 destroyedCount = 0;

    for (const auto& [pendingWorld, entity] : pending) {
        // Only destroy entities belonging to the specified world
        if (pendingWorld != &world) {
            // Re-queue for the correct world
            s_pendingDestruction.emplace_back(pendingWorld, entity);
            s_pendingEntityIds.insert(static_cast<u32>(entity));
            continue;
        }

        if (world.isValid(entity)) {
            world.destroyEntity(entity);
            ++destroyedCount;
        }
    }

    if (destroyedCount > 0) {
        LIMBO_LOG_ECS_DEBUG("Flushed {} deferred entity destructions", destroyedCount);
    }
}

bool DeferredDestruction::isPendingDestruction(World::EntityId entity) {
    u32 rawId = static_cast<u32>(entity);
    return s_pendingEntityIds.find(rawId) != s_pendingEntityIds.end();
}

void DeferredDestruction::clear() {
    s_pendingDestruction.clear();
    s_pendingEntityIds.clear();
}

void DeferredDestruction::beginPhysicsContext() {
    ++s_physicsContextDepth;
}

void DeferredDestruction::endPhysicsContext() {
    if (s_physicsContextDepth > 0) {
        --s_physicsContextDepth;
    }
}

bool DeferredDestruction::isInPhysicsContext() {
    return s_physicsContextDepth > 0;
}

}  // namespace limbo
