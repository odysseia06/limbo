#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/World.hpp"

#include <glm/glm.hpp>

#include <functional>
#include <vector>

namespace limbo {

/**
 * Hierarchy - Utility functions for managing entity parent/child relationships
 *
 * This namespace provides functions to:
 * - Set/clear parent-child relationships
 * - Traverse the hierarchy (children, descendants, ancestors)
 * - Calculate world transforms from local transforms
 * - Safely destroy entities with children
 */
namespace Hierarchy {

/**
 * Set an entity's parent
 * If the entity already has a parent, it will be detached first.
 * The child will be added to the end of the parent's children list.
 *
 * @param world The world containing the entities
 * @param child The entity to become a child
 * @param parent The entity to become the parent (null to make root)
 */
void setParent(World& world, World::EntityId child, World::EntityId parent);

/**
 * Remove an entity from its parent (make it a root entity)
 * @param world The world containing the entity
 * @param entity The entity to detach
 */
void detachFromParent(World& world, World::EntityId entity);

/**
 * Get the parent of an entity
 * @param world The world containing the entity
 * @param entity The entity to query
 * @return The parent entity, or null if root
 */
[[nodiscard]] World::EntityId getParent(const World& world, World::EntityId entity);

/**
 * Get all direct children of an entity
 * @param world The world containing the entity
 * @param entity The entity to query
 * @return Vector of child entity IDs
 */
[[nodiscard]] std::vector<World::EntityId> getChildren(const World& world, World::EntityId entity);

/**
 * Get the number of direct children
 * @param world The world containing the entity
 * @param entity The entity to query
 * @return Number of direct children
 */
[[nodiscard]] u32 getChildCount(const World& world, World::EntityId entity);

/**
 * Check if an entity is an ancestor of another
 * @param world The world containing the entities
 * @param ancestor The potential ancestor
 * @param descendant The potential descendant
 * @return True if ancestor is in descendant's parent chain
 */
<<<<<<< HEAD
[[nodiscard]] bool isAncestorOf(const World& world, World::EntityId ancestor,
                                World::EntityId descendant);
=======
[[nodiscard]] bool isAncestorOf(const World& world, World::EntityId ancestor, World::EntityId descendant);
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0

/**
 * Get the depth of an entity in the hierarchy
 * @param world The world containing the entity
 * @param entity The entity to query
 * @return Depth (0 = root)
 */
[[nodiscard]] u32 getDepth(const World& world, World::EntityId entity);

/**
 * Get the root ancestor of an entity
 * @param world The world containing the entity
 * @param entity The entity to query
 * @return The root entity (may be the entity itself if already root)
 */
[[nodiscard]] World::EntityId getRoot(const World& world, World::EntityId entity);

/**
 * Iterate over all direct children of an entity
 * @param world The world containing the entity
 * @param entity The entity to iterate children of
 * @param callback Function called for each child (return false to stop)
 */
void forEachChild(const World& world, World::EntityId entity,
                  const std::function<bool(World::EntityId)>& callback);

/**
 * Iterate over all descendants of an entity (depth-first)
 * @param world The world containing the entity
 * @param entity The entity to iterate descendants of
 * @param callback Function called for each descendant (return false to stop)
 */
void forEachDescendant(const World& world, World::EntityId entity,
                       const std::function<bool(World::EntityId)>& callback);

/**
 * Iterate over all ancestors of an entity (from parent to root)
 * @param world The world containing the entity
 * @param entity The entity to iterate ancestors of
 * @param callback Function called for each ancestor (return false to stop)
 */
void forEachAncestor(const World& world, World::EntityId entity,
                     const std::function<bool(World::EntityId)>& callback);

/**
 * Calculate the world transform matrix for an entity
 * This multiplies all parent transforms in order from root to entity.
 *
 * @param world The world containing the entity
 * @param entity The entity to calculate world transform for
 * @return The world transform matrix
 */
[[nodiscard]] glm::mat4 getWorldTransform(const World& world, World::EntityId entity);

/**
 * Calculate the world position for an entity
 * @param world The world containing the entity
 * @param entity The entity to query
 * @return The world position
 */
[[nodiscard]] glm::vec3 getWorldPosition(const World& world, World::EntityId entity);

/**
 * Set the world position for an entity (adjusts local position)
 * @param world The world containing the entity
 * @param entity The entity to modify
 * @param worldPos The desired world position
 */
void setWorldPosition(World& world, World::EntityId entity, const glm::vec3& worldPos);

/**
 * Destroy an entity and all its descendants
 * Children are destroyed before parents (depth-first post-order).
 *
 * @param world The world containing the entity
 * @param entity The entity to destroy
 */
void destroyWithChildren(World& world, World::EntityId entity);

/**
 * Reparent all children of an entity to a new parent
 * @param world The world containing the entities
 * @param entity The entity whose children to reparent
 * @param newParent The new parent for the children (null for root)
 */
void reparentChildren(World& world, World::EntityId entity, World::EntityId newParent);

/**
 * Sort children by a comparison function
 * @param world The world containing the entity
 * @param entity The entity whose children to sort
 * @param compare Comparison function (return true if a should come before b)
 */
void sortChildren(World& world, World::EntityId entity,
                  const std::function<bool(World::EntityId, World::EntityId)>& compare);

/**
 * Move a child to a specific index among siblings
 * @param world The world containing the entity
 * @param child The child entity to move
 * @param index The desired index (0 = first child)
 */
void setChildIndex(World& world, World::EntityId child, u32 index);

/**
 * Get the index of a child among its siblings
 * @param world The world containing the entity
 * @param child The child entity
 * @return The index, or -1 if not a child
 */
[[nodiscard]] i32 getChildIndex(const World& world, World::EntityId child);

}  // namespace Hierarchy

}  // namespace limbo
