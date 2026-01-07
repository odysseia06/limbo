#include "limbo/ecs/Hierarchy.hpp"

#include "limbo/core/Assert.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <stack>

namespace limbo::Hierarchy {

namespace {

// Helper to ensure entity has HierarchyComponent
HierarchyComponent& ensureHierarchy(World& world, World::EntityId entity) {
    if (!world.hasComponent<HierarchyComponent>(entity)) {
        return world.addComponent<HierarchyComponent>(entity);
    }
    return world.getComponent<HierarchyComponent>(entity);
}

// Helper to update depth for entity and all descendants
void updateDepthRecursive(World& world, World::EntityId entity, u32 depth) {
    if (!world.isValid(entity)) {
        return;
    }

    auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    if (hierarchy == nullptr) {
        return;
    }

    hierarchy->depth = depth;

    // Update all children
    World::EntityId child = hierarchy->firstChild;
    while (child != World::kNullEntity && world.isValid(child)) {
        updateDepthRecursive(world, child, depth + 1);
        auto* childHierarchy = world.tryGetComponent<HierarchyComponent>(child);
        child = (childHierarchy != nullptr) ? childHierarchy->nextSibling : World::kNullEntity;
    }
}

}  // namespace

void setParent(World& world, World::EntityId child, World::EntityId parent) {
    if (!world.isValid(child)) {
        return;
    }

    // Can't parent to self
    if (child == parent) {
        return;
    }

    // Check for circular reference
    if (parent != World::kNullEntity && isAncestorOf(world, child, parent)) {
        return;
    }

    // Detach from current parent first
    detachFromParent(world, child);

    // If no new parent, we're done (entity is now root)
    if (parent == World::kNullEntity || !world.isValid(parent)) {
        return;
    }

    // Get or create hierarchy components
    HierarchyComponent& childHierarchy = ensureHierarchy(world, child);
    HierarchyComponent& parentHierarchy = ensureHierarchy(world, parent);

    // Set parent reference
    childHierarchy.parent = parent;

    // Add to parent's children list (at end)
    if (parentHierarchy.firstChild == World::kNullEntity) {
        // First child
        parentHierarchy.firstChild = child;
    } else {
        // Find last child and append
        World::EntityId lastChild = parentHierarchy.firstChild;
        auto* lastChildHierarchy = world.tryGetComponent<HierarchyComponent>(lastChild);

        while (lastChildHierarchy != nullptr &&
               lastChildHierarchy->nextSibling != World::kNullEntity) {
            lastChild = lastChildHierarchy->nextSibling;
            lastChildHierarchy = world.tryGetComponent<HierarchyComponent>(lastChild);
        }

        if (lastChildHierarchy != nullptr) {
            lastChildHierarchy->nextSibling = child;
            childHierarchy.prevSibling = lastChild;
        }
    }

    parentHierarchy.childCount++;

    // Update depth for child and all its descendants
    updateDepthRecursive(world, child, parentHierarchy.depth + 1);
}

void detachFromParent(World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return;
    }

    auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    if (hierarchy == nullptr || hierarchy->parent == World::kNullEntity) {
        return;  // No parent to detach from
    }

    World::EntityId parent = hierarchy->parent;
    auto* parentHierarchy = world.tryGetComponent<HierarchyComponent>(parent);

    if (parentHierarchy != nullptr) {
        // Update sibling links
        if (hierarchy->prevSibling != World::kNullEntity) {
            auto* prev = world.tryGetComponent<HierarchyComponent>(hierarchy->prevSibling);
            if (prev != nullptr) {
                prev->nextSibling = hierarchy->nextSibling;
            }
        } else {
            // This was the first child
            parentHierarchy->firstChild = hierarchy->nextSibling;
        }

        if (hierarchy->nextSibling != World::kNullEntity) {
            auto* next = world.tryGetComponent<HierarchyComponent>(hierarchy->nextSibling);
            if (next != nullptr) {
                next->prevSibling = hierarchy->prevSibling;
            }
        }

        parentHierarchy->childCount--;
    }

    // Clear our parent/sibling references
    hierarchy->parent = World::kNullEntity;
    hierarchy->prevSibling = World::kNullEntity;
    hierarchy->nextSibling = World::kNullEntity;

    // Update depth (now root)
    updateDepthRecursive(world, entity, 0);
}

World::EntityId getParent(const World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return World::kNullEntity;
    }

    const auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    return (hierarchy != nullptr) ? hierarchy->parent : World::kNullEntity;
}

std::vector<World::EntityId> getChildren(const World& world, World::EntityId entity) {
    std::vector<World::EntityId> children;

    if (!world.isValid(entity)) {
        return children;
    }

    const auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    if (hierarchy == nullptr) {
        return children;
    }

    children.reserve(hierarchy->childCount);

    World::EntityId child = hierarchy->firstChild;
    while (child != World::kNullEntity && world.isValid(child)) {
        children.push_back(child);
        const auto* childHierarchy = world.tryGetComponent<HierarchyComponent>(child);
        child = (childHierarchy != nullptr) ? childHierarchy->nextSibling : World::kNullEntity;
    }

    return children;
}

u32 getChildCount(const World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return 0;
    }

    const auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    return (hierarchy != nullptr) ? hierarchy->childCount : 0;
}

bool isAncestorOf(const World& world, World::EntityId ancestor, World::EntityId descendant) {
    if (!world.isValid(ancestor) || !world.isValid(descendant)) {
        return false;
    }

    World::EntityId current = getParent(world, descendant);
    while (current != World::kNullEntity) {
        if (current == ancestor) {
            return true;
        }
        current = getParent(world, current);
    }

    return false;
}

u32 getDepth(const World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return 0;
    }

    const auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    return (hierarchy != nullptr) ? hierarchy->depth : 0;
}

World::EntityId getRoot(const World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return World::kNullEntity;
    }

    World::EntityId current = entity;
    World::EntityId parent = getParent(world, current);

    while (parent != World::kNullEntity) {
        current = parent;
        parent = getParent(world, current);
    }

    return current;
}

void forEachChild(const World& world, World::EntityId entity,
                  const std::function<bool(World::EntityId)>& callback) {
    if (!world.isValid(entity)) {
        return;
    }

    const auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    if (hierarchy == nullptr) {
        return;
    }

    World::EntityId child = hierarchy->firstChild;
    while (child != World::kNullEntity && world.isValid(child)) {
        if (!callback(child)) {
            break;
        }
        const auto* childHierarchy = world.tryGetComponent<HierarchyComponent>(child);
        child = (childHierarchy != nullptr) ? childHierarchy->nextSibling : World::kNullEntity;
    }
}

void forEachDescendant(const World& world, World::EntityId entity,
                       const std::function<bool(World::EntityId)>& callback) {
    if (!world.isValid(entity)) {
        return;
    }

    const auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    if (hierarchy == nullptr) {
        return;
    }

    // Depth-first traversal using stack
    std::stack<World::EntityId> stack;

    // Push children in reverse order so first child is processed first
    auto children = getChildren(world, entity);
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        stack.push(*it);
    }

    while (!stack.empty()) {
        World::EntityId current = stack.top();
        stack.pop();

        if (!callback(current)) {
            break;
        }

        // Push this entity's children
        auto currentChildren = getChildren(world, current);
        for (auto it = currentChildren.rbegin(); it != currentChildren.rend(); ++it) {
            stack.push(*it);
        }
    }
}

void forEachAncestor(const World& world, World::EntityId entity,
                     const std::function<bool(World::EntityId)>& callback) {
    World::EntityId current = getParent(world, entity);

    while (current != World::kNullEntity) {
        if (!callback(current)) {
            break;
        }
        current = getParent(world, current);
    }
}

glm::mat4 getWorldTransform(const World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return glm::mat4(1.0f);
    }

    const auto* transform = world.tryGetComponent<TransformComponent>(entity);
    if (transform == nullptr) {
        return glm::mat4(1.0f);
    }

    glm::mat4 localMatrix = transform->getMatrix();

    // If no parent, local is world
    World::EntityId parent = getParent(world, entity);
    if (parent == World::kNullEntity) {
        return localMatrix;
    }

    // Recursively get parent's world transform and multiply
    return getWorldTransform(world, parent) * localMatrix;
}

glm::vec3 getWorldPosition(const World& world, World::EntityId entity) {
    glm::mat4 worldTransform = getWorldTransform(world, entity);
    return glm::vec3(worldTransform[3]);
}

void setWorldPosition(World& world, World::EntityId entity, const glm::vec3& worldPos) {
    if (!world.isValid(entity)) {
        return;
    }

    auto* transform = world.tryGetComponent<TransformComponent>(entity);
    if (transform == nullptr) {
        return;
    }

    World::EntityId parent = getParent(world, entity);
    if (parent == World::kNullEntity) {
        // No parent, world pos = local pos
        transform->position = worldPos;
    } else {
        // Calculate local position from world position
        glm::mat4 parentWorld = getWorldTransform(world, parent);
        glm::mat4 parentWorldInv = glm::inverse(parentWorld);
        glm::vec4 localPos = parentWorldInv * glm::vec4(worldPos, 1.0f);
        transform->position = glm::vec3(localPos);
    }
}

void destroyWithChildren(World& world, World::EntityId entity) {
    if (!world.isValid(entity)) {
        return;
    }

    // Collect all descendants first (depth-first)
    std::vector<World::EntityId> toDestroy;
    toDestroy.push_back(entity);

    forEachDescendant(world, entity, [&toDestroy](World::EntityId descendant) {
        toDestroy.push_back(descendant);
        return true;
    });

    // Destroy in reverse order (children before parents)
    for (auto it = toDestroy.rbegin(); it != toDestroy.rend(); ++it) {
        if (world.isValid(*it)) {
            // Detach from parent to avoid dangling references during destruction
            detachFromParent(world, *it);
            world.destroyEntity(*it);
        }
    }
}

void reparentChildren(World& world, World::EntityId entity, World::EntityId newParent) {
    if (!world.isValid(entity)) {
        return;
    }

    // Get children before modifying
    auto children = getChildren(world, entity);

    for (World::EntityId child : children) {
        setParent(world, child, newParent);
    }
}

void sortChildren(World& world, World::EntityId entity,
                  const std::function<bool(World::EntityId, World::EntityId)>& compare) {
    if (!world.isValid(entity)) {
        return;
    }

    auto* hierarchy = world.tryGetComponent<HierarchyComponent>(entity);
    if (hierarchy == nullptr || hierarchy->childCount < 2) {
        return;
    }

    // Get all children
    auto children = getChildren(world, entity);

    // Sort by comparison
    std::sort(children.begin(), children.end(), compare);

    // Rebuild linked list
    hierarchy->firstChild = children[0];

    for (usize i = 0; i < children.size(); ++i) {
        auto* childHierarchy = world.tryGetComponent<HierarchyComponent>(children[i]);
        if (childHierarchy != nullptr) {
            childHierarchy->prevSibling =
                (i > 0) ? children[i - 1] : World::kNullEntity;
            childHierarchy->nextSibling =
                (i < children.size() - 1) ? children[i + 1] : World::kNullEntity;
        }
    }
}

void setChildIndex(World& world, World::EntityId child, u32 index) {
    if (!world.isValid(child)) {
        return;
    }

    auto* childHierarchy = world.tryGetComponent<HierarchyComponent>(child);
    if (childHierarchy == nullptr || childHierarchy->parent == World::kNullEntity) {
        return;
    }

    World::EntityId parent = childHierarchy->parent;
    auto children = getChildren(world, parent);

    // Find current position
    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end()) {
        return;
    }

    // Remove from current position
    children.erase(it);

    // Insert at new position
    if (index >= children.size()) {
        children.push_back(child);
    } else {
        children.insert(children.begin() + static_cast<std::ptrdiff_t>(index), child);
    }

    // Rebuild linked list
    auto* parentHierarchy = world.tryGetComponent<HierarchyComponent>(parent);
    if (parentHierarchy != nullptr) {
        parentHierarchy->firstChild = children[0];
    }

    for (usize i = 0; i < children.size(); ++i) {
        auto* h = world.tryGetComponent<HierarchyComponent>(children[i]);
        if (h != nullptr) {
            h->prevSibling = (i > 0) ? children[i - 1] : World::kNullEntity;
            h->nextSibling = (i < children.size() - 1) ? children[i + 1] : World::kNullEntity;
        }
    }
}

i32 getChildIndex(const World& world, World::EntityId child) {
    if (!world.isValid(child)) {
        return -1;
    }

    const auto* childHierarchy = world.tryGetComponent<HierarchyComponent>(child);
    if (childHierarchy == nullptr || childHierarchy->parent == World::kNullEntity) {
        return -1;
    }

    auto children = getChildren(world, childHierarchy->parent);
    auto it = std::find(children.begin(), children.end(), child);

    if (it == children.end()) {
        return -1;
    }

    return static_cast<i32>(std::distance(children.begin(), it));
}

}  // namespace limbo::Hierarchy
