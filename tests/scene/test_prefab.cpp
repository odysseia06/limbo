#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Entity.hpp>
#include <limbo/ecs/Components.hpp>
#include <limbo/ecs/Hierarchy.hpp>
#include <limbo/scene/Prefab.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("Prefab: Create from single entity", "[scene][prefab]") {
    limbo::World world;

    auto entity = world.createEntity("TestEntity");
    entity.addComponent<limbo::TransformComponent>(glm::vec3(1.0f, 2.0f, 3.0f));
    entity.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.5f, 0.25f, 1.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, entity.id());

    REQUIRE(prefab.getName() == "TestEntity");
    REQUIRE(prefab.getEntityCount() == 1);
}

TEST_CASE("Prefab: Create from hierarchy", "[scene][prefab]") {
    limbo::World world;

    auto parent = world.createEntity("Parent");
    parent.addComponent<limbo::TransformComponent>(glm::vec3(10.0f, 0.0f, 0.0f));

    auto child1 = world.createEntity("Child1");
    child1.addComponent<limbo::TransformComponent>(glm::vec3(5.0f, 0.0f, 0.0f));
    limbo::Hierarchy::setParent(world, child1.id(), parent.id());

    auto child2 = world.createEntity("Child2");
    child2.addComponent<limbo::TransformComponent>(glm::vec3(0.0f, 5.0f, 0.0f));
    limbo::Hierarchy::setParent(world, child2.id(), parent.id());

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, parent.id());

    REQUIRE(prefab.getName() == "Parent");
    REQUIRE(prefab.getEntityCount() == 3);

    // Verify hierarchy structure in prefab
    const auto& entities = prefab.getEntities();
    REQUIRE(entities[0].parentIndex == -1);  // Parent is root
    REQUIRE(entities[1].parentIndex == 0);   // Child1's parent is index 0
    REQUIRE(entities[2].parentIndex == 0);   // Child2's parent is index 0
}

TEST_CASE("Prefab: Instantiate single entity", "[scene][prefab]") {
    limbo::World world;

    // Create source entity
    auto source = world.createEntity("Source");
    source.addComponent<limbo::TransformComponent>(glm::vec3(5.0f, 10.0f, 15.0f));
    source.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Create prefab
    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());

    // Instantiate in a new world
    limbo::World world2;
    limbo::Entity instance = prefab.instantiate(world2);

    REQUIRE(instance.isValid());
    REQUIRE(world2.entityCount() == 1);

    // Verify components
    REQUIRE(instance.hasComponent<limbo::NameComponent>());
    REQUIRE(instance.getComponent<limbo::NameComponent>().name == "Source");

    REQUIRE(instance.hasComponent<limbo::TransformComponent>());
    auto& transform = instance.getComponent<limbo::TransformComponent>();
    REQUIRE_THAT(transform.position.x, WithinAbs(5.0f, 0.001f));
    REQUIRE_THAT(transform.position.y, WithinAbs(10.0f, 0.001f));

    REQUIRE(instance.hasComponent<limbo::SpriteRendererComponent>());
    auto& sprite = instance.getComponent<limbo::SpriteRendererComponent>();
    REQUIRE_THAT(sprite.color.r, WithinAbs(1.0f, 0.001f));

    // Verify PrefabInstanceComponent
    REQUIRE(instance.hasComponent<limbo::PrefabInstanceComponent>());
    auto& prefabInstance = instance.getComponent<limbo::PrefabInstanceComponent>();
    REQUIRE(prefabInstance.prefabId == prefab.getPrefabId());
    REQUIRE(prefabInstance.isRoot == true);
}

TEST_CASE("Prefab: Instantiate with position offset", "[scene][prefab]") {
    limbo::World world;

    auto source = world.createEntity("Source");
    source.addComponent<limbo::TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());

    limbo::World world2;
    limbo::Entity instance = prefab.instantiate(world2, glm::vec3(100.0f, 200.0f, 0.0f));

    auto& transform = instance.getComponent<limbo::TransformComponent>();
    REQUIRE_THAT(transform.position.x, WithinAbs(100.0f, 0.001f));
    REQUIRE_THAT(transform.position.y, WithinAbs(200.0f, 0.001f));
}

TEST_CASE("Prefab: Instantiate hierarchy", "[scene][prefab]") {
    limbo::World world;

    // Create hierarchy
    auto parent = world.createEntity("Parent");
    parent.addComponent<limbo::TransformComponent>(glm::vec3(10.0f, 0.0f, 0.0f));

    auto child = world.createEntity("Child");
    child.addComponent<limbo::TransformComponent>(glm::vec3(5.0f, 0.0f, 0.0f));
    limbo::Hierarchy::setParent(world, child.id(), parent.id());

    // Create prefab
    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, parent.id());

    // Instantiate
    limbo::World world2;
    limbo::Entity instance = prefab.instantiate(world2);

    REQUIRE(world2.entityCount() == 2);

    // Find child by name
    limbo::World::EntityId childId = limbo::World::kNullEntity;
    world2.each<limbo::NameComponent>([&](limbo::World::EntityId id, limbo::NameComponent& name) {
        if (name.name == "Child") {
            childId = id;
        }
    });

    REQUIRE(childId != limbo::World::kNullEntity);

    // Verify hierarchy is preserved
    REQUIRE(limbo::Hierarchy::getParent(world2, childId) == instance.id());
    REQUIRE(limbo::Hierarchy::getChildCount(world2, instance.id()) == 1);

    // Verify world transform
    glm::vec3 worldPos = limbo::Hierarchy::getWorldPosition(world2, childId);
    REQUIRE_THAT(worldPos.x, WithinAbs(15.0f, 0.001f));  // 10 + 5
}

TEST_CASE("Prefab: Serialize and deserialize", "[scene][prefab]") {
    limbo::World world;

    auto entity = world.createEntity("TestPrefab");
    entity.addComponent<limbo::TransformComponent>(glm::vec3(1.0f, 2.0f, 3.0f));
    entity.addComponent<limbo::SpriteRendererComponent>(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    entity.addComponent<limbo::StaticComponent>();

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, entity.id());
    limbo::UUID originalId = prefab.getPrefabId();

    // Serialize
    limbo::String json = prefab.serialize();
    REQUIRE_FALSE(json.empty());

    // Deserialize into new prefab
    limbo::Prefab prefab2;
    bool success = prefab2.deserialize(json);
    REQUIRE(success);

    REQUIRE(prefab2.getName() == "TestPrefab");
    REQUIRE(prefab2.getPrefabId() == originalId);
    REQUIRE(prefab2.getEntityCount() == 1);

    // Instantiate from loaded prefab
    limbo::World world2;
    limbo::Entity instance = prefab2.instantiate(world2);

    REQUIRE(instance.isValid());
    REQUIRE(instance.hasComponent<limbo::TransformComponent>());
    REQUIRE(instance.hasComponent<limbo::SpriteRendererComponent>());
    REQUIRE(instance.hasComponent<limbo::StaticComponent>());

    auto& transform = instance.getComponent<limbo::TransformComponent>();
    REQUIRE_THAT(transform.position.x, WithinAbs(1.0f, 0.001f));
    REQUIRE_THAT(transform.position.y, WithinAbs(2.0f, 0.001f));
    REQUIRE_THAT(transform.position.z, WithinAbs(3.0f, 0.001f));
}

TEST_CASE("Prefab: Override tracking", "[scene][prefab]") {
    limbo::World world;

    auto entity = world.createEntity("Source");
    entity.addComponent<limbo::TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, entity.id());

    // Instantiate
    limbo::Entity instance = prefab.instantiate(world);

    // Modify the instance
    auto& transform = instance.getComponent<limbo::TransformComponent>();
    transform.position = glm::vec3(100.0f, 0.0f, 0.0f);

    // Mark as override
    auto& prefabInstance = instance.getComponent<limbo::PrefabInstanceComponent>();
    prefabInstance.setOverride("Transform.position");

    REQUIRE(prefabInstance.hasOverride("Transform.position"));
    REQUIRE_FALSE(prefabInstance.hasOverride("Transform.rotation"));

    // Clear override
    prefabInstance.clearOverride("Transform.position");
    REQUIRE_FALSE(prefabInstance.hasOverride("Transform.position"));
}

TEST_CASE("Prefab: Multiple instantiation", "[scene][prefab]") {
    limbo::World world;

    auto entity = world.createEntity("Template");
    entity.addComponent<limbo::TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, entity.id());

    // Create multiple instances
    limbo::World world2;
    auto instance1 = prefab.instantiate(world2, glm::vec3(0.0f, 0.0f, 0.0f));
    auto instance2 = prefab.instantiate(world2, glm::vec3(10.0f, 0.0f, 0.0f));
    auto instance3 = prefab.instantiate(world2, glm::vec3(20.0f, 0.0f, 0.0f));

    REQUIRE(world2.entityCount() == 3);

    // All should be valid and have different positions
    REQUIRE(instance1.isValid());
    REQUIRE(instance2.isValid());
    REQUIRE(instance3.isValid());

    REQUIRE_THAT(instance1.getComponent<limbo::TransformComponent>().position.x,
                 WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(instance2.getComponent<limbo::TransformComponent>().position.x,
                 WithinAbs(10.0f, 0.001f));
    REQUIRE_THAT(instance3.getComponent<limbo::TransformComponent>().position.x,
                 WithinAbs(20.0f, 0.001f));

    // All should reference the same prefab
    REQUIRE(instance1.getComponent<limbo::PrefabInstanceComponent>().prefabId ==
            prefab.getPrefabId());
    REQUIRE(instance2.getComponent<limbo::PrefabInstanceComponent>().prefabId ==
            prefab.getPrefabId());
    REQUIRE(instance3.getComponent<limbo::PrefabInstanceComponent>().prefabId ==
            prefab.getPrefabId());
}
