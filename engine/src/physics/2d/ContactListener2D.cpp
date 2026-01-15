#include "limbo/physics/2d/ContactListener2D.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

void ContactListener2D::BeginContact(b2Contact* contact) {
    enqueueEvent(contact, CollisionEventType::Begin);
}

void ContactListener2D::EndContact(b2Contact* contact) {
    enqueueEvent(contact, CollisionEventType::End);
}

void ContactListener2D::enqueueEvent(b2Contact* contact, CollisionEventType type) {
    if (!contact) {
        return;
    }

    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    if (!fixtureA || !fixtureB) {
        return;
    }

    b2Body* bodyA = fixtureA->GetBody();
    b2Body* bodyB = fixtureB->GetBody();

    if (!bodyA || !bodyB) {
        return;
    }

    // Get entity IDs from body user data
    // Entity IDs are stored as (entity + 1) to distinguish entity 0 from "no entity"
    uintptr_t userDataA = bodyA->GetUserData().pointer;
    uintptr_t userDataB = bodyB->GetUserData().pointer;

    // Both bodies must have entity associations (0 means no entity)
    if (userDataA == 0 || userDataB == 0) {
        return;
    }

    // Decode entity IDs (subtract 1 to get actual entity ID)
    auto entityA = static_cast<World::EntityId>(userDataA - 1);
    auto entityB = static_cast<World::EntityId>(userDataB - 1);

    // Get fixture indices from fixture user data
    i32 fixtureIndexA = static_cast<i32>(fixtureA->GetUserData().pointer);
    i32 fixtureIndexB = static_cast<i32>(fixtureB->GetUserData().pointer);

    // Determine if this is a trigger event
    bool isTrigger = fixtureA->IsSensor() || fixtureB->IsSensor();

    // Get contact normal and point
    glm::vec2 normal{0.0f};
    glm::vec2 contactPoint{0.0f};

    if (type == CollisionEventType::Begin) {
        b2WorldManifold worldManifold;
        contact->GetWorldManifold(&worldManifold);

        // Normal points from A to B
        normal = {worldManifold.normal.x, worldManifold.normal.y};

        // Use first contact point if available
        if (contact->GetManifold()->pointCount > 0) {
            contactPoint = {worldManifold.points[0].x, worldManifold.points[0].y};
        }
    }
    // For EndContact, normal/point are not meaningful (contact is ending)

    PendingEvent event;
    event.entityA = entityA;
    event.entityB = entityB;
    event.normal = normal;
    event.contactPoint = contactPoint;
    event.fixtureIndexA = fixtureIndexA;
    event.fixtureIndexB = fixtureIndexB;
    event.isTrigger = isTrigger;
    event.type = type;

    m_pendingEvents.push_back(event);
}

void ContactListener2D::dispatchEvents() {
    if (!m_callback) {
        m_pendingEvents.clear();
        return;
    }

    // Process all pending events
    // Note: We copy the vector in case callback modifies the listener
    auto events = std::move(m_pendingEvents);
    m_pendingEvents.clear();

    for (const auto& pending : events) {
        // Validate that both entities still exist before dispatching
        // This prevents crashes when entities are destroyed during the physics step
        if (m_world != nullptr) {
            if (!m_world->isValid(pending.entityA) || !m_world->isValid(pending.entityB)) {
                // One or both entities were destroyed, skip this event
                continue;
            }
        }

        // Dispatch to entity A (self=A, other=B, normal from A to B)
        {
            CollisionEvent2D eventA;
            eventA.self = pending.entityA;
            eventA.other = pending.entityB;
            eventA.normal = pending.normal;  // Already points from A to B
            eventA.contactPoint = pending.contactPoint;
            eventA.selfFixtureIndex = pending.fixtureIndexA;
            eventA.otherFixtureIndex = pending.fixtureIndexB;
            eventA.isTrigger = pending.isTrigger;

            m_callback(eventA, pending.type);
        }

        // Re-validate entities after dispatching to A (callback might destroy entities)
        if (m_world != nullptr) {
            if (!m_world->isValid(pending.entityA) || !m_world->isValid(pending.entityB)) {
                // Entity was destroyed during callback, skip dispatch to B
                continue;
            }
        }

        // Dispatch to entity B (self=B, other=A, normal negated to point from B to A)
        {
            CollisionEvent2D eventB;
            eventB.self = pending.entityB;
            eventB.other = pending.entityA;
            eventB.normal = -pending.normal;  // Negate: now points from B to A
            eventB.contactPoint = pending.contactPoint;
            eventB.selfFixtureIndex = pending.fixtureIndexB;
            eventB.otherFixtureIndex = pending.fixtureIndexA;
            eventB.isTrigger = pending.isTrigger;

            m_callback(eventB, pending.type);
        }
    }
}

}  // namespace limbo
