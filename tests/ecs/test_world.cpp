#include <catch2/catch_test_macros.hpp>

#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Entity.hpp>
#include <limbo/ecs/Components.hpp>

TEST_CASE("World entity creation", "[ecs][world]") {
    limbo::World world;

    SECTION("World starts empty") {
        REQUIRE(world.entityCount() == 0);
    }

    SECTION("Create entity increases count") {
        auto entity = world.createEntity();
        REQUIRE(world.entityCount() == 1);
        REQUIRE(world.isValid(entity.id()));
    }

    SECTION("Create named entity") {
        auto entity = world.createEntity("TestEntity");
        REQUIRE(world.entityCount() == 1);
        REQUIRE(world.hasComponent<limbo::NameComponent>(entity.id()));

        auto& name = world.getComponent<limbo::NameComponent>(entity.id());
        REQUIRE(name.name == "TestEntity");
    }

    SECTION("Create multiple entities") {
        auto e1 = world.createEntity("Entity1");
        auto e2 = world.createEntity("Entity2");
        auto e3 = world.createEntity("Entity3");

        REQUIRE(world.entityCount() == 3);
        REQUIRE(e1.id() != e2.id());
        REQUIRE(e2.id() != e3.id());
    }
}

TEST_CASE("World entity destruction", "[ecs][world]") {
    limbo::World world;

    SECTION("Destroy entity decreases count") {
        auto entity = world.createEntity();
        auto id = entity.id();

        REQUIRE(world.entityCount() == 1);
        world.destroyEntity(id);
        REQUIRE(world.entityCount() == 0);
    }

    SECTION("Destroyed entity is invalid") {
        auto entity = world.createEntity();
        auto id = entity.id();

        world.destroyEntity(id);
        REQUIRE_FALSE(world.isValid(id));
    }

    SECTION("Clear removes all entities") {
        world.createEntity("E1");
        world.createEntity("E2");
        world.createEntity("E3");

        REQUIRE(world.entityCount() == 3);
        world.clear();
        REQUIRE(world.entityCount() == 0);
    }
}

TEST_CASE("World component operations", "[ecs][world]") {
    limbo::World world;
    auto entity = world.createEntity();
    auto id = entity.id();

    SECTION("Add and get component") {
        world.addComponent<limbo::TransformComponent>(id, glm::vec3(1.0f, 2.0f, 3.0f));

        REQUIRE(world.hasComponent<limbo::TransformComponent>(id));

        auto& transform = world.getComponent<limbo::TransformComponent>(id);
        REQUIRE(transform.position == glm::vec3(1.0f, 2.0f, 3.0f));
    }

    SECTION("Remove component") {
        world.addComponent<limbo::TransformComponent>(id);
        REQUIRE(world.hasComponent<limbo::TransformComponent>(id));

        world.removeComponent<limbo::TransformComponent>(id);
        REQUIRE_FALSE(world.hasComponent<limbo::TransformComponent>(id));
    }

    SECTION("Try get component returns nullptr when missing") {
        auto* transform = world.tryGetComponent<limbo::TransformComponent>(id);
        REQUIRE(transform == nullptr);
    }

    SECTION("Try get component returns pointer when present") {
        world.addComponent<limbo::TransformComponent>(id);
        auto* transform = world.tryGetComponent<limbo::TransformComponent>(id);
        REQUIRE(transform != nullptr);
    }

    SECTION("Get or add component creates if missing") {
        auto& transform = world.getOrAddComponent<limbo::TransformComponent>(id);
        REQUIRE(world.hasComponent<limbo::TransformComponent>(id));
        transform.position = glm::vec3(5.0f, 5.0f, 5.0f);

        auto& sameTransform = world.getOrAddComponent<limbo::TransformComponent>(id);
        REQUIRE(sameTransform.position == glm::vec3(5.0f, 5.0f, 5.0f));
    }

    SECTION("Has all components") {
        world.addComponent<limbo::TransformComponent>(id);
        world.addComponent<limbo::SpriteRendererComponent>(id);

        REQUIRE(world.hasAllComponents<limbo::TransformComponent, limbo::SpriteRendererComponent>(id));
        REQUIRE_FALSE(world.hasAllComponents<limbo::TransformComponent, limbo::CameraComponent>(id));
    }

    SECTION("Has any component") {
        world.addComponent<limbo::TransformComponent>(id);

        REQUIRE(world.hasAnyComponent<limbo::TransformComponent, limbo::CameraComponent>(id));
        REQUIRE_FALSE(world.hasAnyComponent<limbo::SpriteRendererComponent, limbo::CameraComponent>(id));
    }
}

TEST_CASE("World view iteration", "[ecs][world]") {
    limbo::World world;

    // Create entities with different component combinations
    auto e1 = world.createEntity("WithTransform");
    world.addComponent<limbo::TransformComponent>(e1.id());

    auto e2 = world.createEntity("WithTransformAndSprite");
    world.addComponent<limbo::TransformComponent>(e2.id());
    world.addComponent<limbo::SpriteRendererComponent>(e2.id());

    auto e3 = world.createEntity("WithSpriteOnly");
    world.addComponent<limbo::SpriteRendererComponent>(e3.id());

    SECTION("View returns correct entities") {
        auto transformView = world.view<limbo::TransformComponent>();
        int count = 0;
        for ([[maybe_unused]] auto entity : transformView) {
            count++;
        }
        REQUIRE(count == 2);  // e1 and e2
    }

    SECTION("View with multiple components") {
        auto view = world.view<limbo::TransformComponent, limbo::SpriteRendererComponent>();
        int count = 0;
        for ([[maybe_unused]] auto entity : view) {
            count++;
        }
        REQUIRE(count == 1);  // Only e2
    }

    SECTION("Each iteration") {
        int count = 0;
        world.each<limbo::TransformComponent>([&count](auto entity, auto& transform) {
            (void)entity;
            (void)transform;
            count++;
        });
        REQUIRE(count == 2);
    }
}

TEST_CASE("World tag components", "[ecs][world]") {
    limbo::World world;
    auto entity = world.createEntity();
    auto id = entity.id();

    SECTION("Add empty tag component") {
        world.addComponent<limbo::ActiveComponent>(id);
        REQUIRE(world.hasComponent<limbo::ActiveComponent>(id));
    }

    SECTION("Multiple tag components") {
        world.addComponent<limbo::ActiveComponent>(id);
        world.addComponent<limbo::StaticComponent>(id);

        REQUIRE(world.hasAllComponents<limbo::ActiveComponent, limbo::StaticComponent>(id));
    }
}
