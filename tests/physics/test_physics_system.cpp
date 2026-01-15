#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "limbo/physics/2d/Physics2D.hpp"
#include "limbo/physics/2d/PhysicsSystem2D.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/physics/2d/ContactListener2D.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Components.hpp"

using namespace limbo;

TEST_CASE("PhysicsSystem2D initialization", "[physics][system]") {
    Physics2D physics;
    physics.init({0.0f, -9.81f});

    World world;
    PhysicsSystem2D system(physics);

    SECTION("onAttach sets up contact listener") {
        system.onAttach(world);
        // System should be attached without error
        system.onDetach(world);
    }

    physics.shutdown();
}

TEST_CASE("PhysicsSystem2D body creation", "[physics][system]") {
    Physics2D physics;
    physics.init({0.0f, -9.81f});

    World world;
    PhysicsSystem2D system(physics);
    system.onAttach(world);

    SECTION("entity with rigidbody and collider gets body created") {
        auto entity = world.createEntity();
        world.addComponent<TransformComponent>(entity.id());
        world.addComponent<Rigidbody2DComponent>(entity.id(), BodyType::Dynamic);
        world.addComponent<BoxCollider2DComponent>(entity.id());

        // Run one update to create bodies
        system.update(world, 0.016f);

        auto& rb = world.getComponent<Rigidbody2DComponent>(entity.id());
        REQUIRE(rb.runtimeBody != nullptr);
    }

    SECTION("entity with only rigidbody (no collider) gets body") {
        auto entity = world.createEntity();
        world.addComponent<TransformComponent>(entity.id());
        world.addComponent<Rigidbody2DComponent>(entity.id(), BodyType::Dynamic);

        system.update(world, 0.016f);

        auto& rb = world.getComponent<Rigidbody2DComponent>(entity.id());
        REQUIRE(rb.runtimeBody != nullptr);
    }

    SECTION("body type is set correctly") {
        auto staticEntity = world.createEntity();
        world.addComponent<TransformComponent>(staticEntity.id());
        world.addComponent<Rigidbody2DComponent>(staticEntity.id(), BodyType::Static);

        auto dynamicEntity = world.createEntity();
        world.addComponent<TransformComponent>(dynamicEntity.id());
        world.addComponent<Rigidbody2DComponent>(dynamicEntity.id(), BodyType::Dynamic);

        system.update(world, 0.016f);

        auto& staticRb = world.getComponent<Rigidbody2DComponent>(staticEntity.id());
        auto& dynamicRb = world.getComponent<Rigidbody2DComponent>(dynamicEntity.id());

        REQUIRE(staticRb.runtimeBody->GetType() == b2_staticBody);
        REQUIRE(dynamicRb.runtimeBody->GetType() == b2_dynamicBody);
    }

    system.onDetach(world);
    physics.shutdown();
}

TEST_CASE("PhysicsSystem2D entity destruction", "[physics][system][lifecycle]") {
    Physics2D physics;
    physics.init({0.0f, -9.81f});

    World world;
    PhysicsSystem2D system(physics);
    system.onAttach(world);

    SECTION("destroying entity cleans up physics body") {
        auto entity = world.createEntity();
        auto id = entity.id();
        world.addComponent<TransformComponent>(id);
        world.addComponent<Rigidbody2DComponent>(id, BodyType::Dynamic);
        world.addComponent<BoxCollider2DComponent>(id);

        system.update(world, 0.016f);

        auto& rb = world.getComponent<Rigidbody2DComponent>(id);
        REQUIRE(rb.runtimeBody != nullptr);

        // Destroy the entity
        world.destroyEntity(id);

        // Run another update to ensure no stale references
        system.update(world, 0.016f);
    }

    SECTION("removing rigidbody component cleans up body and fixture pointers") {
        auto entity = world.createEntity();
        auto id = entity.id();
        world.addComponent<TransformComponent>(id);
        world.addComponent<Rigidbody2DComponent>(id, BodyType::Dynamic);
        auto& boxCollider = world.addComponent<BoxCollider2DComponent>(id);

        system.update(world, 0.016f);

        // Verify fixture was created
        REQUIRE(boxCollider.runtimeFixture != nullptr);

        // Remove the rigidbody component
        world.removeComponent<Rigidbody2DComponent>(id);

        // Run update - should not crash
        system.update(world, 0.016f);

        // Entity should still exist but without physics
        REQUIRE(world.isValid(id));

        // Fixture pointer should be cleared to avoid dangling pointer
        auto& colliderAfter = world.getComponent<BoxCollider2DComponent>(id);
        REQUIRE(colliderAfter.runtimeFixture == nullptr);
    }

    SECTION("multiple entities destroyed in same frame") {
        std::vector<World::EntityId> entities;
        for (int i = 0; i < 5; ++i) {
            auto entity = world.createEntity();
            auto id = entity.id();
            world.addComponent<TransformComponent>(id);
            world.addComponent<Rigidbody2DComponent>(id, BodyType::Dynamic);
            world.addComponent<BoxCollider2DComponent>(id);
            entities.push_back(id);
        }

        system.update(world, 0.016f);

        // Destroy all entities
        for (auto id : entities) {
            world.destroyEntity(id);
        }

        // Update should handle all cleanups
        system.update(world, 0.016f);
    }

    system.onDetach(world);
    physics.shutdown();
}

TEST_CASE("PhysicsSystem2D collision callbacks", "[physics][system][collisions]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});  // No gravity for predictable tests

    World world;
    PhysicsSystem2D system(physics);
    system.onAttach(world);

    // Track collision events
    std::vector<CollisionEvent2D> collisionEvents;
    system.setCollisionCallback(
        [&collisionEvents](const CollisionEvent2D& event, CollisionEventType type) {
            if (type == CollisionEventType::Begin) {
                collisionEvents.push_back(event);
            }
        });

    SECTION("collision between two bodies fires callback") {
        // Create two bodies that will collide - positioned close together
        auto entityA = world.createEntity();
        auto idA = entityA.id();
        world.addComponent<TransformComponent>(idA, glm::vec3(0.0f, 0.0f, 0.0f));
        auto& rbA = world.addComponent<Rigidbody2DComponent>(idA, BodyType::Dynamic);
        rbA.linearVelocity = {5.0f, 0.0f};  // Moving right fast
        world.addComponent<BoxCollider2DComponent>(idA, glm::vec2(0.5f, 0.5f));

        auto entityB = world.createEntity();
        auto idB = entityB.id();
        // Position B close enough that collision will happen quickly
        world.addComponent<TransformComponent>(idB, glm::vec3(1.2f, 0.0f, 0.0f));
        world.addComponent<Rigidbody2DComponent>(idB, BodyType::Dynamic);
        world.addComponent<BoxCollider2DComponent>(idB, glm::vec2(0.5f, 0.5f));

        // Run physics steps - collision should happen within first few frames
        for (int i = 0; i < 60; ++i) {
            system.update(world, 0.016f);
            if (collisionEvents.size() > 0) {
                break;  // Got collision, can stop early
            }
        }

        // Should have received collision events
        REQUIRE(collisionEvents.size() > 0);
    }

    system.onDetach(world);
    physics.shutdown();
}

TEST_CASE("PhysicsSystem2D stale contact handling", "[physics][system][lifecycle]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});

    World world;
    PhysicsSystem2D system(physics);
    system.onAttach(world);

    std::vector<CollisionEvent2D> collisionEvents;
    system.setCollisionCallback(
        [&collisionEvents](const CollisionEvent2D& event, CollisionEventType type) {
            if (type == CollisionEventType::Begin) {
                collisionEvents.push_back(event);
            }
        });

    SECTION("destroying entity during contact doesn't crash") {
        // Create two colliding bodies
        auto entityA = world.createEntity();
        auto idA = entityA.id();
        world.addComponent<TransformComponent>(idA, glm::vec3(0.0f, 0.0f, 0.0f));
        auto& rbA = world.addComponent<Rigidbody2DComponent>(idA, BodyType::Dynamic);
        rbA.linearVelocity = {2.0f, 0.0f};
        world.addComponent<BoxCollider2DComponent>(idA, glm::vec2(0.5f, 0.5f));

        auto entityB = world.createEntity();
        auto idB = entityB.id();
        world.addComponent<TransformComponent>(idB, glm::vec3(1.5f, 0.0f, 0.0f));
        world.addComponent<Rigidbody2DComponent>(idB, BodyType::Dynamic);
        world.addComponent<BoxCollider2DComponent>(idB, glm::vec2(0.5f, 0.5f));

        // Let them collide
        for (int i = 0; i < 30; ++i) {
            system.update(world, 0.016f);
        }

        // Destroy one entity while potentially in contact
        world.destroyEntity(idA);

        // Continue simulation - should not crash or fire stale events
        collisionEvents.clear();
        for (int i = 0; i < 30; ++i) {
            system.update(world, 0.016f);
        }

        // Any collision events should reference valid entities
        for (const auto& event : collisionEvents) {
            REQUIRE(world.isValid(event.self));
            REQUIRE(world.isValid(event.other));
        }
    }

    SECTION("destroying both entities during contact doesn't crash") {
        auto entityA = world.createEntity();
        auto idA = entityA.id();
        world.addComponent<TransformComponent>(idA, glm::vec3(0.0f, 0.0f, 0.0f));
        auto& rbA = world.addComponent<Rigidbody2DComponent>(idA, BodyType::Dynamic);
        rbA.linearVelocity = {2.0f, 0.0f};
        world.addComponent<BoxCollider2DComponent>(idA, glm::vec2(0.5f, 0.5f));

        auto entityB = world.createEntity();
        auto idB = entityB.id();
        world.addComponent<TransformComponent>(idB, glm::vec3(1.5f, 0.0f, 0.0f));
        world.addComponent<Rigidbody2DComponent>(idB, BodyType::Dynamic);
        world.addComponent<BoxCollider2DComponent>(idB, glm::vec2(0.5f, 0.5f));

        // Let them collide
        for (int i = 0; i < 30; ++i) {
            system.update(world, 0.016f);
        }

        // Destroy both entities
        world.destroyEntity(idA);
        world.destroyEntity(idB);

        // Continue simulation - should not crash
        for (int i = 0; i < 30; ++i) {
            system.update(world, 0.016f);
        }
    }

    system.onDetach(world);
    physics.shutdown();
}

TEST_CASE("PhysicsSystem2D fixed timestep", "[physics][system]") {
    Physics2D physics;
    physics.init({0.0f, -9.81f});

    World world;
    PhysicsSystem2D system(physics);
    system.setFixedTimestep(1.0f / 60.0f);
    system.onAttach(world);

    SECTION("fixed timestep configuration") {
        REQUIRE_THAT(system.getFixedTimestep(), Catch::Matchers::WithinAbs(1.0f / 60.0f, 0.0001f));

        system.setFixedTimestep(1.0f / 120.0f);
        REQUIRE_THAT(system.getFixedTimestep(), Catch::Matchers::WithinAbs(1.0f / 120.0f, 0.0001f));
    }

    SECTION("interpolation can be toggled") {
        REQUIRE(system.isInterpolationEnabled());

        system.setInterpolationEnabled(false);
        REQUIRE_FALSE(system.isInterpolationEnabled());

        system.setInterpolationEnabled(true);
        REQUIRE(system.isInterpolationEnabled());
    }

    system.onDetach(world);
    physics.shutdown();
}
