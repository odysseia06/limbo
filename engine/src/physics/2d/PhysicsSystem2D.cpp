#include "limbo/physics/2d/PhysicsSystem2D.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/DeferredDestruction.hpp"
#include "limbo/debug/Log.hpp"

namespace limbo {

PhysicsSystem2D::PhysicsSystem2D(Physics2D& physics) : m_physics(physics) {}

void PhysicsSystem2D::onAttach(World& world) {
    // Register contact listener with Box2D world
    if (m_physics.isInitialized()) {
        m_physics.getWorld()->SetContactListener(&m_contactListener);
    }

    // Create bodies for all existing entities with physics components
    auto view = world.view<TransformComponent, Rigidbody2DComponent>();
    for (auto entity : view) {
        createBody(world, entity);

        // Initialize physics state for interpolation
        auto& rb = view.get<Rigidbody2DComponent>(entity);
        if (rb.runtimeBody != nullptr) {
            const b2Vec2& pos = rb.runtimeBody->GetPosition();
            PhysicsState state;
            state.previousPosition = {pos.x, pos.y};
            state.previousRotation = rb.runtimeBody->GetAngle();
            state.currentPosition = state.previousPosition;
            state.currentRotation = state.previousRotation;
            m_physicsStates[entity] = state;
        }
    }

    LIMBO_LOG_PHYSICS_DEBUG("PhysicsSystem initialized with {} bodies", m_physicsStates.size());
}

void PhysicsSystem2D::update(World& world, f32 deltaTime) {
    if (!m_physics.isInitialized()) {
        return;
    }

    // Create bodies for new entities (those without runtime body)
    {
        auto view = world.view<TransformComponent, Rigidbody2DComponent>();
        for (auto entity : view) {
            auto& rb = view.get<Rigidbody2DComponent>(entity);
            if (rb.runtimeBody == nullptr) {
                createBody(world, entity);

                // Initialize physics state for new entity
                const b2Vec2& pos = rb.runtimeBody->GetPosition();
                PhysicsState state;
                state.previousPosition = {pos.x, pos.y};
                state.previousRotation = rb.runtimeBody->GetAngle();
                state.currentPosition = state.previousPosition;
                state.currentRotation = state.previousRotation;
                m_physicsStates[entity] = state;
            }
        }
    }

    // Fixed timestep accumulator
    m_accumulator += deltaTime;

    // Spiral-of-death protection: clamp to max updates
    i32 updateCount = 0;
    if (m_accumulator > m_fixedTimestep * static_cast<f32>(m_maxFixedUpdatesPerFrame)) {
        LIMBO_LOG_PHYSICS_WARN("Physics: clamping {} accumulated updates to max {}",
                               static_cast<i32>(m_accumulator / m_fixedTimestep),
                               m_maxFixedUpdatesPerFrame);
        m_accumulator = m_fixedTimestep * static_cast<f32>(m_maxFixedUpdatesPerFrame);
    }

    // Run fixed updates
    while (m_accumulator >= m_fixedTimestep) {
        fixedUpdate(world);
        m_accumulator -= m_fixedTimestep;
        updateCount++;
    }

    // Calculate interpolation alpha (how far we are between physics states)
    f32 const alpha = m_accumulator / m_fixedTimestep;

    // Write interpolated render state to TransformComponent
    // Note: This NEVER affects simulation - it's purely for rendering
    if (m_interpolationEnabled) {
        interpolateRenderState(world, alpha);
    } else {
        // No interpolation: just copy current state to transform
        interpolateRenderState(world, 1.0f);
    }
}

void PhysicsSystem2D::onDetach(World& world) {
    // Clear contact listener
    if (m_physics.isInitialized()) {
        m_physics.getWorld()->SetContactListener(nullptr);
    }

    // Destroy all physics bodies
    auto view = world.view<Rigidbody2DComponent>();
    for (auto entity : view) {
        destroyBody(world, entity);
    }

    // Clear physics state cache
    m_physicsStates.clear();

    LIMBO_LOG_PHYSICS_DEBUG("PhysicsSystem shutdown");
}

void PhysicsSystem2D::fixedUpdate(World& world) {
    // 1. Snapshot previous = current
    snapshotPreviousState(world);

    // 2. Step Box2D
    m_physics.step(m_fixedTimestep);

    // 3. Dispatch collision events AFTER step completes
    // Use physics context so entity destruction is deferred until after dispatch
    {
        DeferredDestruction::ScopedPhysicsContext physicsContext;
        m_contactListener.dispatchEvents();
    }

    // 4. Flush any deferred entity destructions (safe now that dispatch is complete)
    DeferredDestruction::flush(world);

    // 5. Read current from bodies (do NOT write to TransformComponent)
    readCurrentStateFromBodies(world);
}

void PhysicsSystem2D::snapshotPreviousState(World& world) {
    for (auto& [entity, state] : m_physicsStates) {
        state.previousPosition = state.currentPosition;
        state.previousRotation = state.currentRotation;
    }
}

void PhysicsSystem2D::readCurrentStateFromBodies(World& world) {
    auto view = world.view<Rigidbody2DComponent>();
    for (auto entity : view) {
        auto& rb = view.get<Rigidbody2DComponent>(entity);
        if (!rb.runtimeBody || rb.type == BodyType::Static) {
            continue;
        }

        auto it = m_physicsStates.find(entity);
        if (it == m_physicsStates.end()) {
            continue;
        }

        const b2Vec2& pos = rb.runtimeBody->GetPosition();
        it->second.currentPosition = {pos.x, pos.y};
        it->second.currentRotation = rb.runtimeBody->GetAngle();
    }
}

void PhysicsSystem2D::interpolateRenderState(World& world, f32 alpha) {
    auto view = world.view<TransformComponent, Rigidbody2DComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rb = view.get<Rigidbody2DComponent>(entity);

        if (!rb.runtimeBody || rb.type == BodyType::Static) {
            continue;
        }

        auto it = m_physicsStates.find(entity);
        if (it == m_physicsStates.end()) {
            continue;
        }

        const auto& state = it->second;

        // Interpolate position: lerp(previous, current, alpha)
        transform.position.x =
            state.previousPosition.x + alpha * (state.currentPosition.x - state.previousPosition.x);
        transform.position.y =
            state.previousPosition.y + alpha * (state.currentPosition.y - state.previousPosition.y);

        // Interpolate rotation (simple lerp for small angles)
        // For large angle differences, should use proper angle interpolation
        f32 rotDiff = state.currentRotation - state.previousRotation;

        // Normalize angle difference to [-PI, PI]
        while (rotDiff > glm::pi<f32>()) {
            rotDiff -= 2.0f * glm::pi<f32>();
        }
        while (rotDiff < -glm::pi<f32>()) {
            rotDiff += 2.0f * glm::pi<f32>();
        }

        transform.rotation.z = state.previousRotation + alpha * rotDiff;
    }
}

void PhysicsSystem2D::setCollisionCallback(CollisionCallback callback) {
    m_contactListener.setCallback(std::move(callback));
}

void PhysicsSystem2D::createBody(World& world, World::EntityId entity) {
    if (!m_physics.isInitialized()) {
        return;
    }

    auto& transform = world.getComponent<TransformComponent>(entity);
    auto& rb = world.getComponent<Rigidbody2DComponent>(entity);

    // Don't create if already exists
    if (rb.runtimeBody != nullptr) {
        return;
    }

    b2World* b2world = m_physics.getWorld();

    // Create body definition
    b2BodyDef bodyDef;
    bodyDef.position.Set(transform.position.x, transform.position.y);
    bodyDef.angle = transform.rotation.z;

    switch (rb.type) {
    case BodyType::Static:
        bodyDef.type = b2_staticBody;
        break;
    case BodyType::Kinematic:
        bodyDef.type = b2_kinematicBody;
        break;
    case BodyType::Dynamic:
        bodyDef.type = b2_dynamicBody;
        break;
    }

    bodyDef.gravityScale = rb.gravityScale;
    bodyDef.fixedRotation = rb.fixedRotation;
    bodyDef.linearVelocity.Set(rb.linearVelocity.x, rb.linearVelocity.y);
    bodyDef.angularVelocity = rb.angularVelocity;
    bodyDef.linearDamping = rb.linearDamping;
    bodyDef.angularDamping = rb.angularDamping;

    // Create the body
    rb.runtimeBody = b2world->CreateBody(&bodyDef);

    // Store entity ID in body user data for collision callbacks
    rb.runtimeBody->GetUserData().pointer = static_cast<uintptr_t>(entity);

    // Track fixture index for multiple colliders per entity
    i32 fixtureIndex = 0;
    bool hasCollider = false;

    // Create fixtures for colliders
    // Box collider
    if (world.hasComponent<BoxCollider2DComponent>(entity)) {
        hasCollider = true;
        auto& box = world.getComponent<BoxCollider2DComponent>(entity);

        b2PolygonShape boxShape;
        // box.size is already half-extents, SetAsBox expects half-extents
        boxShape.SetAsBox(box.size.x * transform.scale.x, box.size.y * transform.scale.y,
                          b2Vec2(box.offset.x, box.offset.y), 0.0f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &boxShape;
        fixtureDef.density = box.density;
        fixtureDef.friction = box.friction;
        fixtureDef.restitution = box.restitution;
        fixtureDef.restitutionThreshold = box.restitutionThreshold;
        fixtureDef.isSensor = box.isTrigger;

        box.runtimeFixture = rb.runtimeBody->CreateFixture(&fixtureDef);

        // Store fixture index in fixture user data
        box.runtimeFixture->GetUserData().pointer = static_cast<uintptr_t>(fixtureIndex);
        fixtureIndex++;
    }

    // Circle collider
    if (world.hasComponent<CircleCollider2DComponent>(entity)) {
        hasCollider = true;
        auto& circle = world.getComponent<CircleCollider2DComponent>(entity);

        b2CircleShape circleShape;
        circleShape.m_p.Set(circle.offset.x, circle.offset.y);
        circleShape.m_radius = circle.radius * glm::max(transform.scale.x, transform.scale.y);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.density = circle.density;
        fixtureDef.friction = circle.friction;
        fixtureDef.restitution = circle.restitution;
        fixtureDef.restitutionThreshold = circle.restitutionThreshold;
        fixtureDef.isSensor = circle.isTrigger;

        circle.runtimeFixture = rb.runtimeBody->CreateFixture(&fixtureDef);

        // Store fixture index in fixture user data
        circle.runtimeFixture->GetUserData().pointer = static_cast<uintptr_t>(fixtureIndex);
        fixtureIndex++;
    }

    // If no collider components exist, create a default box fixture based on transform scale
    // This ensures the rigidbody has mass and participates in physics simulation
    if (!hasCollider && rb.type == BodyType::Dynamic) {
        b2PolygonShape defaultShape;
        // Use half the transform scale as the box half-extents (matching sprite size)
        defaultShape.SetAsBox(transform.scale.x * 0.5f, transform.scale.y * 0.5f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &defaultShape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.3f;
        fixtureDef.restitution = 0.0f;

        rb.runtimeBody->CreateFixture(&fixtureDef);

        LIMBO_LOG_PHYSICS_DEBUG(
            "Created default fixture for Rigidbody2D without collider (entity {})",
            static_cast<u32>(entity));
    }
}

void PhysicsSystem2D::destroyBody(World& world, World::EntityId entity) {
    if (!m_physics.isInitialized()) {
        return;
    }

    auto& rb = world.getComponent<Rigidbody2DComponent>(entity);
    if (rb.runtimeBody) {
        m_physics.getWorld()->DestroyBody(rb.runtimeBody);
        rb.runtimeBody = nullptr;
    }

    // Clear fixture references
    if (world.hasComponent<BoxCollider2DComponent>(entity)) {
        world.getComponent<BoxCollider2DComponent>(entity).runtimeFixture = nullptr;
    }
    if (world.hasComponent<CircleCollider2DComponent>(entity)) {
        world.getComponent<CircleCollider2DComponent>(entity).runtimeFixture = nullptr;
    }

    // Remove physics state
    m_physicsStates.erase(entity);
}

void PhysicsSystem2D::syncTransformToBody(World& world, World::EntityId entity) {
    auto& transform = world.getComponent<TransformComponent>(entity);
    auto& rb = world.getComponent<Rigidbody2DComponent>(entity);

    if (rb.runtimeBody) {
        rb.runtimeBody->SetTransform(b2Vec2(transform.position.x, transform.position.y),
                                     transform.rotation.z);
    }
}

}  // namespace limbo
