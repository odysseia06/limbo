#include <catch2/catch_test_macros.hpp>

#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Hierarchy.hpp"
#include "limbo/ecs/World.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

using namespace limbo;

TEST_CASE("Hierarchy: Basic parent-child relationships", "[ecs][hierarchy]") {
    World world;

    Entity parent = world.createEntity("Parent");
    Entity child = world.createEntity("Child");

    SECTION("Set parent creates relationship") {
        Hierarchy::setParent(world, child.id(), parent.id());

        REQUIRE(Hierarchy::getParent(world, child.id()) == parent.id());
        REQUIRE(Hierarchy::getChildCount(world, parent.id()) == 1);

        auto children = Hierarchy::getChildren(world, parent.id());
        REQUIRE(children.size() == 1);
        REQUIRE(children[0] == child.id());
    }

    SECTION("Detach from parent makes root") {
        Hierarchy::setParent(world, child.id(), parent.id());
        Hierarchy::detachFromParent(world, child.id());

        REQUIRE(Hierarchy::getParent(world, child.id()) == World::kNullEntity);
        REQUIRE(Hierarchy::getChildCount(world, parent.id()) == 0);
    }

    SECTION("Cannot parent to self") {
        Hierarchy::setParent(world, parent.id(), parent.id());
        REQUIRE(Hierarchy::getParent(world, parent.id()) == World::kNullEntity);
    }

    SECTION("Reparenting detaches from old parent") {
        Entity newParent = world.createEntity("NewParent");

        Hierarchy::setParent(world, child.id(), parent.id());
        REQUIRE(Hierarchy::getChildCount(world, parent.id()) == 1);

        Hierarchy::setParent(world, child.id(), newParent.id());
        REQUIRE(Hierarchy::getChildCount(world, parent.id()) == 0);
        REQUIRE(Hierarchy::getChildCount(world, newParent.id()) == 1);
        REQUIRE(Hierarchy::getParent(world, child.id()) == newParent.id());
    }
}

TEST_CASE("Hierarchy: Multiple children", "[ecs][hierarchy]") {
    World world;

    Entity parent = world.createEntity("Parent");
    Entity child1 = world.createEntity("Child1");
    Entity child2 = world.createEntity("Child2");
    Entity child3 = world.createEntity("Child3");

    Hierarchy::setParent(world, child1.id(), parent.id());
    Hierarchy::setParent(world, child2.id(), parent.id());
    Hierarchy::setParent(world, child3.id(), parent.id());

    SECTION("All children tracked") {
        REQUIRE(Hierarchy::getChildCount(world, parent.id()) == 3);

        auto children = Hierarchy::getChildren(world, parent.id());
        REQUIRE(children.size() == 3);
        REQUIRE(children[0] == child1.id());
        REQUIRE(children[1] == child2.id());
        REQUIRE(children[2] == child3.id());
    }

    SECTION("Removing middle child updates siblings") {
        Hierarchy::detachFromParent(world, child2.id());

        REQUIRE(Hierarchy::getChildCount(world, parent.id()) == 2);

        auto children = Hierarchy::getChildren(world, parent.id());
        REQUIRE(children.size() == 2);
        REQUIRE(children[0] == child1.id());
        REQUIRE(children[1] == child3.id());
    }

    SECTION("forEachChild iterates all") {
        std::vector<World::EntityId> visited;
        Hierarchy::forEachChild(world, parent.id(), [&](World::EntityId id) {
            visited.push_back(id);
            return true;
        });

        REQUIRE(visited.size() == 3);
        REQUIRE(visited[0] == child1.id());
        REQUIRE(visited[1] == child2.id());
        REQUIRE(visited[2] == child3.id());
    }

    SECTION("forEachChild can stop early") {
        std::vector<World::EntityId> visited;
        Hierarchy::forEachChild(world, parent.id(), [&](World::EntityId id) {
            visited.push_back(id);
            return visited.size() < 2;  // Stop after 2
        });

        REQUIRE(visited.size() == 2);
    }
}

TEST_CASE("Hierarchy: Deep nesting", "[ecs][hierarchy]") {
    World world;

    Entity root = world.createEntity("Root");
    Entity level1 = world.createEntity("Level1");
    Entity level2 = world.createEntity("Level2");
    Entity level3 = world.createEntity("Level3");

    Hierarchy::setParent(world, level1.id(), root.id());
    Hierarchy::setParent(world, level2.id(), level1.id());
    Hierarchy::setParent(world, level3.id(), level2.id());

    SECTION("Depth is tracked") {
        REQUIRE(Hierarchy::getDepth(world, root.id()) == 0);
        REQUIRE(Hierarchy::getDepth(world, level1.id()) == 1);
        REQUIRE(Hierarchy::getDepth(world, level2.id()) == 2);
        REQUIRE(Hierarchy::getDepth(world, level3.id()) == 3);
    }

    SECTION("isAncestorOf works") {
        REQUIRE(Hierarchy::isAncestorOf(world, root.id(), level3.id()));
        REQUIRE(Hierarchy::isAncestorOf(world, level1.id(), level3.id()));
        REQUIRE(Hierarchy::isAncestorOf(world, level2.id(), level3.id()));
        REQUIRE_FALSE(Hierarchy::isAncestorOf(world, level3.id(), root.id()));
        REQUIRE_FALSE(Hierarchy::isAncestorOf(world, level2.id(), level1.id()));
    }

    SECTION("getRoot returns root") {
        REQUIRE(Hierarchy::getRoot(world, level3.id()) == root.id());
        REQUIRE(Hierarchy::getRoot(world, level1.id()) == root.id());
        REQUIRE(Hierarchy::getRoot(world, root.id()) == root.id());
    }

    SECTION("Cannot create circular hierarchy") {
        // Try to make root a child of level3 (would create a cycle)
        Hierarchy::setParent(world, root.id(), level3.id());

        // Should still be root with no parent
        REQUIRE(Hierarchy::getParent(world, root.id()) == World::kNullEntity);
    }

    SECTION("forEachDescendant visits all") {
        std::vector<World::EntityId> visited;
        Hierarchy::forEachDescendant(world, root.id(), [&](World::EntityId id) {
            visited.push_back(id);
            return true;
        });

        REQUIRE(visited.size() == 3);
        // Depth-first order
        REQUIRE(visited[0] == level1.id());
        REQUIRE(visited[1] == level2.id());
        REQUIRE(visited[2] == level3.id());
    }

    SECTION("forEachAncestor visits all") {
        std::vector<World::EntityId> visited;
        Hierarchy::forEachAncestor(world, level3.id(), [&](World::EntityId id) {
            visited.push_back(id);
            return true;
        });

        REQUIRE(visited.size() == 3);
        // Parent to root order
        REQUIRE(visited[0] == level2.id());
        REQUIRE(visited[1] == level1.id());
        REQUIRE(visited[2] == root.id());
    }

    SECTION("Depth updates when reparenting") {
        // Move level2 to be direct child of root
        Hierarchy::setParent(world, level2.id(), root.id());

        REQUIRE(Hierarchy::getDepth(world, level2.id()) == 1);
        REQUIRE(Hierarchy::getDepth(world, level3.id()) == 2);  // Child of level2
    }
}

TEST_CASE("Hierarchy: World transforms", "[ecs][hierarchy]") {
    World world;

    Entity parent = world.createEntity("Parent");
    Entity child = world.createEntity("Child");

    parent.addComponent<TransformComponent>(glm::vec3(10.0f, 0.0f, 0.0f));
    child.addComponent<TransformComponent>(glm::vec3(5.0f, 0.0f, 0.0f));

    Hierarchy::setParent(world, child.id(), parent.id());

    SECTION("World position includes parent") {
        glm::vec3 worldPos = Hierarchy::getWorldPosition(world, child.id());

        // Parent at 10, child local at 5 = world 15
        REQUIRE(glm::epsilonEqual(worldPos.x, 15.0f, 0.001f));
        REQUIRE(glm::epsilonEqual(worldPos.y, 0.0f, 0.001f));
        REQUIRE(glm::epsilonEqual(worldPos.z, 0.0f, 0.001f));
    }

    SECTION("World transform matrix includes parent") {
        glm::mat4 worldTransform = Hierarchy::getWorldTransform(world, child.id());
        glm::vec3 worldPos = glm::vec3(worldTransform[3]);

        REQUIRE(glm::epsilonEqual(worldPos.x, 15.0f, 0.001f));
    }

    SECTION("Set world position adjusts local") {
        Hierarchy::setWorldPosition(world, child.id(), glm::vec3(20.0f, 0.0f, 0.0f));

        auto& transform = child.getComponent<TransformComponent>();
        // World 20, parent at 10 = local should be 10
        REQUIRE(glm::epsilonEqual(transform.position.x, 10.0f, 0.001f));
    }

    SECTION("Root entity world pos equals local pos") {
        glm::vec3 worldPos = Hierarchy::getWorldPosition(world, parent.id());

        REQUIRE(glm::epsilonEqual(worldPos.x, 10.0f, 0.001f));
    }
}

TEST_CASE("Hierarchy: Destroy with children", "[ecs][hierarchy]") {
    World world;

    Entity root = world.createEntity("Root");
    Entity child1 = world.createEntity("Child1");
    Entity child2 = world.createEntity("Child2");
    Entity grandchild = world.createEntity("Grandchild");

    Hierarchy::setParent(world, child1.id(), root.id());
    Hierarchy::setParent(world, child2.id(), root.id());
    Hierarchy::setParent(world, grandchild.id(), child1.id());

    REQUIRE(world.entityCount() == 4);

    Hierarchy::destroyWithChildren(world, root.id());

    REQUIRE(world.entityCount() == 0);
}

TEST_CASE("Hierarchy: Child ordering", "[ecs][hierarchy]") {
    World world;

    Entity parent = world.createEntity("Parent");
    Entity child1 = world.createEntity("Child1");
    Entity child2 = world.createEntity("Child2");
    Entity child3 = world.createEntity("Child3");

    Hierarchy::setParent(world, child1.id(), parent.id());
    Hierarchy::setParent(world, child2.id(), parent.id());
    Hierarchy::setParent(world, child3.id(), parent.id());

    SECTION("getChildIndex returns correct index") {
        REQUIRE(Hierarchy::getChildIndex(world, child1.id()) == 0);
        REQUIRE(Hierarchy::getChildIndex(world, child2.id()) == 1);
        REQUIRE(Hierarchy::getChildIndex(world, child3.id()) == 2);
    }

    SECTION("setChildIndex moves child") {
        Hierarchy::setChildIndex(world, child3.id(), 0);

        auto children = Hierarchy::getChildren(world, parent.id());
        REQUIRE(children[0] == child3.id());
        REQUIRE(children[1] == child1.id());
        REQUIRE(children[2] == child2.id());
    }

    SECTION("sortChildren reorders") {
        // Sort by entity ID descending (child3 > child2 > child1)
        Hierarchy::sortChildren(world, parent.id(), [](World::EntityId a, World::EntityId b) {
            return static_cast<u32>(a) > static_cast<u32>(b);
        });

        auto children = Hierarchy::getChildren(world, parent.id());
        REQUIRE(children[0] == child3.id());
        REQUIRE(children[1] == child2.id());
        REQUIRE(children[2] == child1.id());
    }
}

TEST_CASE("Hierarchy: Reparent children", "[ecs][hierarchy]") {
    World world;

    Entity oldParent = world.createEntity("OldParent");
    Entity newParent = world.createEntity("NewParent");
    Entity child1 = world.createEntity("Child1");
    Entity child2 = world.createEntity("Child2");

    Hierarchy::setParent(world, child1.id(), oldParent.id());
    Hierarchy::setParent(world, child2.id(), oldParent.id());

    Hierarchy::reparentChildren(world, oldParent.id(), newParent.id());

    REQUIRE(Hierarchy::getChildCount(world, oldParent.id()) == 0);
    REQUIRE(Hierarchy::getChildCount(world, newParent.id()) == 2);
    REQUIRE(Hierarchy::getParent(world, child1.id()) == newParent.id());
    REQUIRE(Hierarchy::getParent(world, child2.id()) == newParent.id());
}
