#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Entity.hpp>
#include <limbo/ecs/Components.hpp>
#include <limbo/ecs/Hierarchy.hpp>
#include <limbo/scene/SceneSerializer.hpp>
#include <limbo/scene/Prefab.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("SceneSerializer empty world", "[scene][serialization]") {
    limbo::World world;
    limbo::SceneSerializer serializer(world);

    SECTION("Serialize empty world") {
        limbo::String json = serializer.serialize();
        REQUIRE_FALSE(json.empty());
    }

    SECTION("Deserialize empty scene") {
        limbo::String json = serializer.serialize();

        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);

        bool success = serializer2.deserialize(json);
        REQUIRE(success);
        REQUIRE(world2.entityCount() == 0);
    }
}

TEST_CASE("SceneSerializer roundtrip with entities", "[scene][serialization]") {
    limbo::World world;

    // Create test entities
    auto e1 = world.createEntity("TestEntity1");
    world.addComponent<limbo::TransformComponent>(e1.id(), glm::vec3(1.0f, 2.0f, 3.0f),
                                                  glm::vec3(0.1f, 0.2f, 0.3f),
                                                  glm::vec3(1.5f, 1.5f, 1.5f));

    auto e2 = world.createEntity("TestEntity2");
    world.addComponent<limbo::TransformComponent>(e2.id(), glm::vec3(10.0f, 20.0f, 30.0f));
    world.addComponent<limbo::SpriteRendererComponent>(e2.id(), glm::vec4(1.0f, 0.5f, 0.25f, 1.0f));

    limbo::SceneSerializer serializer(world);
    limbo::String json = serializer.serialize();

    SECTION("JSON is not empty") {
        REQUIRE_FALSE(json.empty());
    }

    SECTION("Roundtrip preserves entity count") {
        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);

        bool success = serializer2.deserialize(json);
        REQUIRE(success);
        REQUIRE(world2.entityCount() == 2);
    }

    SECTION("Roundtrip preserves NameComponent") {
        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);
        serializer2.deserialize(json);

        bool foundEntity1 = false;
        bool foundEntity2 = false;

        world2.each<limbo::NameComponent>([&](auto, auto& name) {
            if (name.name == "TestEntity1")
                foundEntity1 = true;
            if (name.name == "TestEntity2")
                foundEntity2 = true;
        });

        REQUIRE(foundEntity1);
        REQUIRE(foundEntity2);
    }

    SECTION("Roundtrip preserves TransformComponent") {
        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);
        serializer2.deserialize(json);

        bool foundCorrectTransform = false;

        world2.each<limbo::NameComponent, limbo::TransformComponent>(
            [&](auto, auto& name, auto& transform) {
                if (name.name == "TestEntity1") {
                    if (transform.position == glm::vec3(1.0f, 2.0f, 3.0f) &&
                        transform.scale == glm::vec3(1.5f, 1.5f, 1.5f)) {
                        foundCorrectTransform = true;
                    }
                }
            });

        REQUIRE(foundCorrectTransform);
    }

    SECTION("Roundtrip preserves SpriteRendererComponent") {
        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);
        serializer2.deserialize(json);

        bool foundSprite = false;

        world2.each<limbo::NameComponent, limbo::SpriteRendererComponent>([&](auto, auto& name,
                                                                              auto& sprite) {
            if (name.name == "TestEntity2") {
                if (sprite.color.r > 0.99f && sprite.color.g > 0.49f && sprite.color.g < 0.51f) {
                    foundSprite = true;
                }
            }
        });

        REQUIRE(foundSprite);
    }
}

TEST_CASE("SceneSerializer handles invalid input", "[scene][serialization]") {
    limbo::World world;
    limbo::SceneSerializer serializer(world);

    SECTION("Empty string returns false") {
        bool success = serializer.deserialize("");
        REQUIRE_FALSE(success);
    }

    SECTION("Invalid JSON returns false") {
        bool success = serializer.deserialize("{ not valid json }}}");
        REQUIRE_FALSE(success);
    }

    SECTION("Error message is set on failure") {
        serializer.deserialize("invalid");
        REQUIRE_FALSE(serializer.getError().empty());
    }
}

// Note: CameraComponent serialization is not yet implemented in SceneSerializer
// This test will be enabled once CameraComponent serialization is added
TEST_CASE("SceneSerializer preserves entity with multiple components", "[scene][serialization]") {
    limbo::World world;

    auto entity = world.createEntity("ComplexEntity");
    world.addComponent<limbo::TransformComponent>(entity.id(), glm::vec3(5.0f, 10.0f, 15.0f),
                                                  glm::vec3(0.0f, 0.0f, glm::radians(45.0f)),
                                                  glm::vec3(2.0f, 2.0f, 2.0f));
    world.addComponent<limbo::SpriteRendererComponent>(entity.id(),
                                                       glm::vec4(0.8f, 0.2f, 0.5f, 1.0f));
    world.addComponent<limbo::StaticComponent>(entity.id());

    limbo::SceneSerializer serializer(world);
    limbo::String json = serializer.serialize();

    limbo::World world2;
    limbo::SceneSerializer serializer2(world2);
    bool success = serializer2.deserialize(json);
    REQUIRE(success);

    bool found = false;
    world2.each<limbo::NameComponent, limbo::TransformComponent, limbo::SpriteRendererComponent>(
        [&](auto, auto& name, auto& transform, auto& sprite) {
            if (name.name == "ComplexEntity") {
                found = true;
                REQUIRE_THAT(transform.position.x, WithinAbs(5.0f, 0.001f));
                REQUIRE_THAT(transform.position.y, WithinAbs(10.0f, 0.001f));
                REQUIRE_THAT(transform.scale.x, WithinAbs(2.0f, 0.001f));
                REQUIRE_THAT(sprite.color.r, WithinAbs(0.8f, 0.001f));
            }
        });

    REQUIRE(found);

    // Also verify the Static tag component
    bool hasStatic = false;
    auto staticView = world2.view<limbo::NameComponent, limbo::StaticComponent>();
    for (auto e : staticView) {
        auto& name = staticView.get<limbo::NameComponent>(e);
        if (name.name == "ComplexEntity") {
            hasStatic = true;
        }
    }
    REQUIRE(hasStatic);
}

TEST_CASE("SceneSerializer preserves hierarchy", "[scene][serialization]") {
    limbo::World world;

    // Create a hierarchy: Parent -> Child1, Child2 -> Grandchild
    auto parent = world.createEntity("Parent");
    world.addComponent<limbo::TransformComponent>(parent.id(), glm::vec3(10.0f, 0.0f, 0.0f));

    auto child1 = world.createEntity("Child1");
    world.addComponent<limbo::TransformComponent>(child1.id(), glm::vec3(5.0f, 0.0f, 0.0f));
    limbo::Hierarchy::setParent(world, child1.id(), parent.id());

    auto child2 = world.createEntity("Child2");
    world.addComponent<limbo::TransformComponent>(child2.id(), glm::vec3(0.0f, 5.0f, 0.0f));
    limbo::Hierarchy::setParent(world, child2.id(), parent.id());

    auto grandchild = world.createEntity("Grandchild");
    world.addComponent<limbo::TransformComponent>(grandchild.id(), glm::vec3(2.0f, 2.0f, 0.0f));
    limbo::Hierarchy::setParent(world, grandchild.id(), child1.id());

    limbo::SceneSerializer serializer(world);
    limbo::String json = serializer.serialize();

    SECTION("Roundtrip preserves hierarchy structure") {
        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);
        bool success = serializer2.deserialize(json);
        REQUIRE(success);
        REQUIRE(world2.entityCount() == 4);

        // Find entities by name
        limbo::World::EntityId parentId = limbo::World::kNullEntity;
        limbo::World::EntityId child1Id = limbo::World::kNullEntity;
        limbo::World::EntityId child2Id = limbo::World::kNullEntity;
        limbo::World::EntityId grandchildId = limbo::World::kNullEntity;

<<<<<<< HEAD
        world2.each<limbo::NameComponent>(
            [&](limbo::World::EntityId id, limbo::NameComponent& name) {
                if (name.name == "Parent")
                    parentId = id;
                if (name.name == "Child1")
                    child1Id = id;
                if (name.name == "Child2")
                    child2Id = id;
                if (name.name == "Grandchild")
                    grandchildId = id;
            });
=======
        world2.each<limbo::NameComponent>([&](limbo::World::EntityId id, limbo::NameComponent& name) {
            if (name.name == "Parent") parentId = id;
            if (name.name == "Child1") child1Id = id;
            if (name.name == "Child2") child2Id = id;
            if (name.name == "Grandchild") grandchildId = id;
        });
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0

        REQUIRE(parentId != limbo::World::kNullEntity);
        REQUIRE(child1Id != limbo::World::kNullEntity);
        REQUIRE(child2Id != limbo::World::kNullEntity);
        REQUIRE(grandchildId != limbo::World::kNullEntity);

        // Verify parent is root
        REQUIRE(limbo::Hierarchy::getParent(world2, parentId) == limbo::World::kNullEntity);

        // Verify children of parent
        REQUIRE(limbo::Hierarchy::getParent(world2, child1Id) == parentId);
        REQUIRE(limbo::Hierarchy::getParent(world2, child2Id) == parentId);
        REQUIRE(limbo::Hierarchy::getChildCount(world2, parentId) == 2);

        // Verify grandchild
        REQUIRE(limbo::Hierarchy::getParent(world2, grandchildId) == child1Id);
        REQUIRE(limbo::Hierarchy::getChildCount(world2, child1Id) == 1);
    }

    SECTION("World transforms are correct after load") {
        limbo::World world2;
        limbo::SceneSerializer serializer2(world2);
        serializer2.deserialize(json);

        limbo::World::EntityId grandchildId = limbo::World::kNullEntity;
<<<<<<< HEAD
        world2.each<limbo::NameComponent>(
            [&](limbo::World::EntityId id, limbo::NameComponent& name) {
                if (name.name == "Grandchild")
                    grandchildId = id;
            });
=======
        world2.each<limbo::NameComponent>([&](limbo::World::EntityId id, limbo::NameComponent& name) {
            if (name.name == "Grandchild") grandchildId = id;
        });
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0

        REQUIRE(grandchildId != limbo::World::kNullEntity);

        // Grandchild world position: Parent(10,0,0) + Child1(5,0,0) + Grandchild(2,2,0) = (17,2,0)
        glm::vec3 worldPos = limbo::Hierarchy::getWorldPosition(world2, grandchildId);
        REQUIRE_THAT(worldPos.x, WithinAbs(17.0f, 0.001f));
        REQUIRE_THAT(worldPos.y, WithinAbs(2.0f, 0.001f));
    }
}

TEST_CASE("SceneSerializer preserves CameraComponent", "[scene][serialization]") {
    limbo::World world;

    auto cameraEntity = world.createEntity("MainCamera");
    world.addComponent<limbo::TransformComponent>(cameraEntity.id());
    auto& camera = world.addComponent<limbo::CameraComponent>(cameraEntity.id());
    camera.projectionType = limbo::CameraComponent::ProjectionType::Orthographic;
    camera.orthoSize = 10.0f;
    camera.nearClip = 0.01f;
    camera.farClip = 100.0f;
    camera.primary = true;

    limbo::SceneSerializer serializer(world);
    limbo::String json = serializer.serialize();

    limbo::World world2;
    limbo::SceneSerializer serializer2(world2);
    bool success = serializer2.deserialize(json);
    REQUIRE(success);

    bool found = false;
    world2.each<limbo::NameComponent, limbo::CameraComponent>(
        [&](auto, limbo::NameComponent& name, limbo::CameraComponent& cam) {
            if (name.name == "MainCamera") {
                found = true;
                REQUIRE(cam.projectionType == limbo::CameraComponent::ProjectionType::Orthographic);
                REQUIRE_THAT(cam.orthoSize, WithinAbs(10.0f, 0.001f));
                REQUIRE_THAT(cam.nearClip, WithinAbs(0.01f, 0.001f));
                REQUIRE_THAT(cam.farClip, WithinAbs(100.0f, 0.001f));
                REQUIRE(cam.primary == true);
            }
        });

    REQUIRE(found);
}

TEST_CASE("SceneSerializer preserves PrefabInstanceComponent with overrides",
          "[scene][serialization]") {
    limbo::World world;

    // Create a prefab and instantiate it
    auto source = world.createEntity("Template");
    source.addComponent<limbo::TransformComponent>(glm::vec3(5.0f, 0.0f, 0.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());
    limbo::Entity instance = prefab.instantiate(world, glm::vec3(10.0f, 0.0f, 0.0f));

    // Set some overrides
    auto& prefabInstance = instance.getComponent<limbo::PrefabInstanceComponent>();
    prefabInstance.setOverride("Transform.position");
    prefabInstance.setOverride("SpriteRenderer.color");

    limbo::SceneSerializer serializer(world);
    limbo::String json = serializer.serialize();

    // Deserialize into new world
    limbo::World world2;
    limbo::SceneSerializer serializer2(world2);
    bool success = serializer2.deserialize(json);
    REQUIRE(success);

    // Find the prefab instance
    bool found = false;
    world2.each<limbo::NameComponent, limbo::PrefabInstanceComponent>(
        [&](auto, limbo::NameComponent& name, limbo::PrefabInstanceComponent& inst) {
            if (name.name == "Template") {
                found = true;
                // Verify prefab ID matches
                REQUIRE(inst.prefabId == prefab.getPrefabId());
                REQUIRE(inst.entityIndex == 0);
                REQUIRE(inst.isRoot == true);

                // Verify overrides survived
                REQUIRE(inst.hasOverride("Transform.position"));
                REQUIRE(inst.hasOverride("SpriteRenderer.color"));
                REQUIRE_FALSE(inst.hasOverride("Transform.rotation"));
            }
        });

    REQUIRE(found);
}
