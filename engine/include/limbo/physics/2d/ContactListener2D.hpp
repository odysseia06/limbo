#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/World.hpp"

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include <functional>
#include <vector>

namespace limbo {

/**
 * CollisionEvent2D - Describes a collision from one entity's perspective
 *
 * The event is "self-relative": the normal points from self toward other,
 * making it easy for scripts to determine collision direction.
 */
struct LIMBO_API CollisionEvent2D {
    World::EntityId self = World::kNullEntity;
    World::EntityId other = World::kNullEntity;
    glm::vec2 normal{0.0f};        // Points from self toward other
    glm::vec2 contactPoint{0.0f};  // World-space contact point
    i32 selfFixtureIndex = 0;      // Which collider on self
    i32 otherFixtureIndex = 0;     // Which collider on other
    bool isTrigger = false;        // True if either fixture is a sensor
};

/**
 * CollisionEventType - Type of collision event
 */
enum class CollisionEventType : u8 {
    Begin,  // Contact started (onCollisionBegin / onTriggerEnter)
    End     // Contact ended (onCollisionEnd / onTriggerExit)
};

/**
 * Collision callback signature
 * @param event The collision event with self-relative data
 * @param type Begin or End
 */
using CollisionCallback =
    std::function<void(const CollisionEvent2D& event, CollisionEventType type)>;

/**
 * ContactListener2D - Box2D contact listener that buffers events
 *
 * IMPORTANT: This listener only enqueues events during Step().
 * Events must be dispatched AFTER Step() completes to avoid
 * crashes from scripts modifying the world during iteration.
 *
 * Usage:
 *   ContactListener2D listener;
 *   listener.setCallback([](const CollisionEvent2D& e, CollisionEventType t) {
 *       // Handle event safely - world step is complete
 *   });
 *   b2World->SetContactListener(&listener);
 *
 *   // In update:
 *   b2World->Step(...);
 *   listener.dispatchEvents();  // Now safe to modify world
 */
class LIMBO_API ContactListener2D : public b2ContactListener {
public:
    ContactListener2D() = default;
    ~ContactListener2D() override = default;

    /**
     * Set the callback for collision events
     * Called after Step() during dispatchEvents()
     */
    void setCallback(CollisionCallback callback) { m_callback = std::move(callback); }

    /**
     * Set the world reference for entity validation
     * Events involving destroyed entities will be filtered out
     */
    void setWorld(World* world) { m_world = world; }

    /**
     * Dispatch all buffered events and clear the queue
     * MUST be called after b2World::Step() completes
     */
    void dispatchEvents();

    /**
     * Clear pending events without dispatching
     */
    void clearEvents() { m_pendingEvents.clear(); }

    /**
     * Get the number of pending events
     */
    [[nodiscard]] usize pendingEventCount() const { return m_pendingEvents.size(); }

protected:
    // b2ContactListener overrides - only enqueue, never dispatch
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;

private:
    struct PendingEvent {
        World::EntityId entityA = World::kNullEntity;
        World::EntityId entityB = World::kNullEntity;
        glm::vec2 normal{0.0f};  // From A toward B
        glm::vec2 contactPoint{0.0f};
        i32 fixtureIndexA = 0;
        i32 fixtureIndexB = 0;
        bool isTrigger = false;
        CollisionEventType type = CollisionEventType::Begin;
    };

    void enqueueEvent(b2Contact* contact, CollisionEventType type);

    std::vector<PendingEvent> m_pendingEvents;
    CollisionCallback m_callback;
    World* m_world = nullptr;
};

}  // namespace limbo
