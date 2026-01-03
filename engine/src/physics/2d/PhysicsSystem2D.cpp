#include "limbo/physics/2d/PhysicsSystem2D.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

PhysicsSystem2D::PhysicsSystem2D(Physics2D& physics) : m_physics(physics) {}

void PhysicsSystem2D::onAttach(World& world) {
    // Create bodies for all existing entities with physics components
    auto view = world.view<TransformComponent, Rigidbody2DComponent>();
    for (auto entity : view) {
        createBody(world, entity);
    }

    spdlog::debug("PhysicsSystem initialized");
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
            }
        }
    }

    // Step the physics simulation
    m_physics.step(deltaTime);

    // Sync physics bodies back to transforms
    {
        auto view = world.view<TransformComponent, Rigidbody2DComponent>();
        for (auto entity : view) {
            syncBodyToTransform(world, entity);
        }
    }
}

void PhysicsSystem2D::onDetach(World& world) {
    // Destroy all physics bodies
    auto view = world.view<Rigidbody2DComponent>();
    for (auto entity : view) {
        destroyBody(world, entity);
    }

    spdlog::debug("PhysicsSystem shutdown");
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

    // Create fixtures for colliders
    // Box collider
    if (world.hasComponent<BoxCollider2DComponent>(entity)) {
        auto& box = world.getComponent<BoxCollider2DComponent>(entity);

        b2PolygonShape boxShape;
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
    }

    // Circle collider
    if (world.hasComponent<CircleCollider2DComponent>(entity)) {
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
}

void PhysicsSystem2D::syncTransformToBody(World& world, World::EntityId entity) {
    auto& transform = world.getComponent<TransformComponent>(entity);
    auto& rb = world.getComponent<Rigidbody2DComponent>(entity);

    if (rb.runtimeBody) {
        rb.runtimeBody->SetTransform(b2Vec2(transform.position.x, transform.position.y),
                                     transform.rotation.z);
    }
}

void PhysicsSystem2D::syncBodyToTransform(World& world, World::EntityId entity) {
    auto& transform = world.getComponent<TransformComponent>(entity);
    auto& rb = world.getComponent<Rigidbody2DComponent>(entity);

    if (rb.runtimeBody && rb.type != BodyType::Static) {
        const b2Vec2& position = rb.runtimeBody->GetPosition();
        transform.position.x = position.x;
        transform.position.y = position.y;
        transform.rotation.z = rb.runtimeBody->GetAngle();
    }
}

}  // namespace limbo
