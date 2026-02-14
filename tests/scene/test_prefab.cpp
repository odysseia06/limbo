#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <nlohmann/json.hpp>

#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Entity.hpp>
#include <limbo/ecs/Components.hpp>
#include <limbo/ecs/Hierarchy.hpp>
#include <limbo/physics/2d/PhysicsComponents2D.hpp>
#include <limbo/scripting/ScriptComponent.hpp>
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

    // Verify hierarchy structure in prefab (using local IDs)
    const auto& entities = prefab.getEntities();
    REQUIRE(entities[0].isRoot());  // Parent is root (empty parentLocalId)
    REQUIRE(entities[1].parentLocalId == entities[0].localId);  // Child1's parent is root
    REQUIRE(entities[2].parentLocalId == entities[0].localId);  // Child2's parent is root
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

    // Mark as override with actual value
    auto& prefabInstance = instance.getComponent<limbo::PrefabInstanceComponent>();
    nlohmann::json positionValue = {{"x", 100.0f}, {"y", 0.0f}, {"z", 0.0f}};
    prefabInstance.setOverride("Transform", "position", positionValue);

    REQUIRE(prefabInstance.hasOverride("Transform", "position"));
    REQUIRE_FALSE(prefabInstance.hasOverride("Transform", "rotation"));

    // Legacy API still works
    REQUIRE(prefabInstance.hasOverride("Transform.position"));
    REQUIRE_FALSE(prefabInstance.hasOverride("Transform.rotation"));

    // Clear override
    prefabInstance.clearOverride("Transform", "position");
    REQUIRE_FALSE(prefabInstance.hasOverride("Transform", "position"));
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

TEST_CASE("Prefab: updateInstances syncs SpriteRenderer", "[scene][prefab]") {
    limbo::World world;

    // Create source entity with sprite
    auto source = world.createEntity("Sprite");
    source.addComponent<limbo::TransformComponent>();
    source.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());

    // Instantiate
    limbo::World world2;
    auto instance = prefab.instantiate(world2);

    REQUIRE(instance.hasComponent<limbo::SpriteRendererComponent>());
    auto& sprite = instance.getComponent<limbo::SpriteRendererComponent>();
    REQUIRE_THAT(sprite.color.r, WithinAbs(1.0f, 0.001f));

    // Modify the prefab source color
    auto* prefabEntity = prefab.findEntity("root");
    REQUIRE(prefabEntity != nullptr);
    prefabEntity->components["SpriteRenderer"]["color"] =
        nlohmann::json::array({0.0f, 1.0f, 0.0f, 1.0f});

    // Update instances
    prefab.updateInstances(world2, false);

    auto& updatedSprite = instance.getComponent<limbo::SpriteRendererComponent>();
    REQUIRE_THAT(updatedSprite.color.r, WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(updatedSprite.color.g, WithinAbs(1.0f, 0.001f));
}

TEST_CASE("Prefab: updateInstances syncs BoxCollider2D", "[scene][prefab]") {
    limbo::World world;

    auto source = world.createEntity("Physics");
    source.addComponent<limbo::TransformComponent>();
    source.addComponent<limbo::BoxCollider2DComponent>(glm::vec2(1.0f, 1.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());

    limbo::World world2;
    auto instance = prefab.instantiate(world2);

    REQUIRE(instance.hasComponent<limbo::BoxCollider2DComponent>());
    auto& box = instance.getComponent<limbo::BoxCollider2DComponent>();
    REQUIRE_THAT(box.size.x, WithinAbs(1.0f, 0.001f));

    // Modify the prefab
    auto* prefabEntity = prefab.findEntity("root");
    REQUIRE(prefabEntity != nullptr);
    prefabEntity->components["BoxCollider2D"]["size"] = nlohmann::json::array({2.0f, 3.0f});
    prefabEntity->components["BoxCollider2D"]["friction"] = 0.8f;

    prefab.updateInstances(world2, false);

    auto& updatedBox = instance.getComponent<limbo::BoxCollider2DComponent>();
    REQUIRE_THAT(updatedBox.size.x, WithinAbs(2.0f, 0.001f));
    REQUIRE_THAT(updatedBox.size.y, WithinAbs(3.0f, 0.001f));
    REQUIRE_THAT(updatedBox.friction, WithinAbs(0.8f, 0.001f));
}

TEST_CASE("Prefab: updateInstances respects overrides", "[scene][prefab]") {
    limbo::World world;

    auto source = world.createEntity("Source");
    source.addComponent<limbo::TransformComponent>(glm::vec3(1.0f, 2.0f, 3.0f));
    source.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());

    limbo::World world2;
    auto instance = prefab.instantiate(world2);

    // Mark color as overridden on the instance
    auto& prefabInst = instance.getComponent<limbo::PrefabInstanceComponent>();
    prefabInst.setOverride("SpriteRenderer", "color",
                           nlohmann::json::array({0.0f, 0.0f, 1.0f, 1.0f}));

    // Set instance color to blue (the override value)
    instance.getComponent<limbo::SpriteRendererComponent>().color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    // Change prefab to green
    auto* prefabEntity = prefab.findEntity("root");
    prefabEntity->components["SpriteRenderer"]["color"] =
        nlohmann::json::array({0.0f, 1.0f, 0.0f, 1.0f});

    // Update with respectOverrides = true
    prefab.updateInstances(world2, true);

    // Color should stay blue (overridden), not change to green
    auto& sprite = instance.getComponent<limbo::SpriteRendererComponent>();
    REQUIRE_THAT(sprite.color.b, WithinAbs(1.0f, 0.001f));
    REQUIRE_THAT(sprite.color.g, WithinAbs(0.0f, 0.001f));

    // But sortingLayer should update (not overridden)
    prefabEntity->components["SpriteRenderer"]["sortingLayer"] = 5;
    prefab.updateInstances(world2, true);
    REQUIRE(instance.getComponent<limbo::SpriteRendererComponent>().sortingLayer == 5);
}

TEST_CASE("Prefab: updateInstances syncs Script component", "[scene][prefab]") {
    limbo::World world;

    auto source = world.createEntity("Scripted");
    source.addComponent<limbo::TransformComponent>();
    auto& script = source.addComponent<limbo::ScriptComponent>();
    script.scriptPath = "scripts/player.lua";
    script.enabled = true;

    limbo::Prefab prefab = limbo::Prefab::createFromEntity(world, source.id());

    limbo::World world2;
    auto instance = prefab.instantiate(world2);

    REQUIRE(instance.hasComponent<limbo::ScriptComponent>());
    REQUIRE(instance.getComponent<limbo::ScriptComponent>().scriptPath == "scripts/player.lua");

    // Modify prefab script path
    auto* prefabEntity = prefab.findEntity("root");
    prefabEntity->components["Script"]["scriptPath"] = "scripts/enemy.lua";

    prefab.updateInstances(world2, false);

    REQUIRE(instance.getComponent<limbo::ScriptComponent>().scriptPath == "scripts/enemy.lua");
}
