# Scenes and Prefabs

This document describes the scene system, entity hierarchy, prefabs, and serialization in Limbo Engine.

## Scene System Overview

Scenes in Limbo are collections of entities managed by the `World` class (ECS). Each entity can have components that define its behavior and appearance.

### Key Components

| Component | Purpose |
|-----------|---------|
| `NameComponent` | Human-readable entity name |
| `TransformComponent` | Position, rotation, scale in 3D space |
| `HierarchyComponent` | Parent/child relationships |
| `SpriteRendererComponent` | 2D sprite rendering |
| `CameraComponent` | Camera projection settings |
| `StaticComponent` | Tag for static entities |
| `ActiveComponent` | Tag for active/enabled entities |

## Entity Hierarchy

Entities can be organized in parent/child hierarchies. Child transforms are relative to their parent.

### HierarchyComponent

```cpp
struct HierarchyComponent {
    entt::entity parent = entt::null;       // Parent entity
    entt::entity firstChild = entt::null;   // First child in linked list
    entt::entity nextSibling = entt::null;  // Next sibling
    entt::entity prevSibling = entt::null;  // Previous sibling
    u32 childCount = 0;                     // Number of direct children
    u32 depth = 0;                          // Depth in hierarchy (0 = root)
};
```

### Hierarchy API

The `Hierarchy` namespace provides functions for managing parent/child relationships:

```cpp
#include <limbo/ecs/Hierarchy.hpp>

// Set parent (detaches from previous parent if any)
Hierarchy::setParent(world, child, parent);

// Remove from parent (make root)
Hierarchy::detachFromParent(world, entity);

// Query relationships
World::EntityId parent = Hierarchy::getParent(world, entity);
std::vector<World::EntityId> children = Hierarchy::getChildren(world, entity);
u32 childCount = Hierarchy::getChildCount(world, entity);

// Hierarchy checks
bool isAncestor = Hierarchy::isAncestorOf(world, ancestor, descendant);
u32 depth = Hierarchy::getDepth(world, entity);
World::EntityId root = Hierarchy::getRoot(world, entity);

// Iteration
Hierarchy::forEachChild(world, entity, [](World::EntityId child) {
    // Process child
    return true;  // Continue iteration
});

Hierarchy::forEachDescendant(world, entity, callback);  // Depth-first
Hierarchy::forEachAncestor(world, entity, callback);    // Parent to root

// World transforms (includes parent transforms)
glm::mat4 worldTransform = Hierarchy::getWorldTransform(world, entity);
glm::vec3 worldPos = Hierarchy::getWorldPosition(world, entity);
Hierarchy::setWorldPosition(world, entity, glm::vec3(x, y, z));

// Destruction
Hierarchy::destroyWithChildren(world, entity);  // Destroys entity and all descendants
Hierarchy::reparentChildren(world, entity, newParent);  // Move children to new parent

// Ordering
Hierarchy::sortChildren(world, entity, compareFunc);
Hierarchy::setChildIndex(world, child, index);
i32 index = Hierarchy::getChildIndex(world, child);
```

## Scene Serialization

Scenes are serialized to JSON format using `SceneSerializer`.

### Usage

```cpp
#include <limbo/scene/SceneSerializer.hpp>

// Save scene
SceneSerializer serializer(world);
serializer.saveToFile("scenes/level1.json");

// Load scene (clears existing entities)
serializer.loadFromFile("scenes/level1.json");

// String serialization
String json = serializer.serialize();
serializer.deserialize(json);
```

### JSON Format (Version 2)

```json
{
  "version": 2,
  "engine": "Limbo",
  "entities": [
    {
      "name": "Player",
      "components": {
        "Transform": {
          "position": [0, 0, 0],
          "rotation": [0, 0, 0],
          "scale": [1, 1, 1]
        },
        "Hierarchy": {
          "parent": 0
        },
        "SpriteRenderer": {
          "color": [1, 1, 1, 1],
          "sortingOrder": 0,
          "textureId": "uuid-string",
          "uvMin": [0, 0],
          "uvMax": [1, 1]
        },
        "Camera": {
          "projectionType": "orthographic",
          "fov": 0.785,
          "orthoSize": 5,
          "nearClip": 0.1,
          "farClip": 1000,
          "primary": true
        },
        "Static": {},
        "Active": {}
      }
    }
  ]
}
```

### Schema Versioning

The serialization system supports automatic schema migration:

```cpp
#include <limbo/scene/SchemaMigration.hpp>

// Migrations are applied automatically during deserialization
// Current version is kSceneFormatVersion (2)

// Custom migrations can be registered:
SchemaMigration migration;
migration.registerMigration(2, [](json& data) {
    // Migrate from v2 to v3
    // Modify data in place
    return true;  // Success
});

migration.migrate(data, fromVersion, toVersion);
```

## Prefabs

Prefabs are reusable entity templates that can be instantiated multiple times.

### Creating a Prefab

```cpp
#include <limbo/scene/Prefab.hpp>

// Create prefab from existing entity (includes children)
Entity sourceEntity = world.createEntity("Enemy");
sourceEntity.addComponent<TransformComponent>();
sourceEntity.addComponent<SpriteRendererComponent>();

Prefab prefab = Prefab::createFromEntity(world, sourceEntity.id());

// Save to file
prefab.saveToFile("prefabs/enemy.prefab");
```

### Instantiating a Prefab

```cpp
// Load prefab
Prefab prefab;
prefab.loadFromFile("prefabs/enemy.prefab");

// Instantiate at different positions
Entity enemy1 = prefab.instantiate(world, glm::vec3(0, 0, 0));
Entity enemy2 = prefab.instantiate(world, glm::vec3(10, 0, 0));
Entity enemy3 = prefab.instantiate(world, glm::vec3(20, 0, 0));
```

### Prefab Instances

Instantiated entities have a `PrefabInstanceComponent` that tracks:
- Which prefab they came from (`prefabId`)
- Their index in the prefab hierarchy (`entityIndex`)
- Whether they're the root of the instance (`isRoot`)
- Property overrides

```cpp
auto& instance = entity.getComponent<PrefabInstanceComponent>();

// Check/set overrides
if (!instance.hasOverride("Transform.position")) {
    // Position matches prefab
}

instance.setOverride("Transform.position");  // Mark as overridden
instance.clearOverride("Transform.position"); // Clear override
instance.clearAllOverrides();  // Revert to prefab values
```

### Updating Prefab Instances

```cpp
// Update all instances to match prefab (respects overrides)
prefab.updateInstances(world, true);

// Force update all instances (ignores overrides)
prefab.updateInstances(world, false);

// Revert specific instance to prefab
prefab.revertInstance(world, instanceRoot);

// Apply instance changes back to prefab
prefab.applyInstanceChanges(world, instanceRoot);
```

### Prefab JSON Format

```json
{
  "version": 1,
  "type": "Prefab",
  "name": "Enemy",
  "id": "uuid-string",
  "entities": [
    {
      "name": "Enemy",
      "parentIndex": -1,
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
      },
      "spriteRenderer": {
        "color": [1, 0, 0, 1]
      }
    },
    {
      "name": "Weapon",
      "parentIndex": 0,
      "transform": {
        "position": [1, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [0.5, 0.5, 0.5]
      }
    }
  ]
}
```

## Editor Integration

### Scene Hierarchy Panel

The editor's Scene Hierarchy panel supports:
- Tree view with expandable parent/child structure
- Drag-and-drop reparenting
- Context menu actions:
  - Create Empty Entity
  - Create Child
  - Duplicate
  - Delete (with children)
  - Unparent

### Prefab Workflow

1. Create entities in the scene
2. Right-click root entity > "Create Prefab" (future feature)
3. Prefab saved to `prefabs/` directory
4. Drag prefab from Asset Browser to instantiate (future feature)

## Best Practices

### Scene Organization

1. Use meaningful entity names
2. Group related entities under parent containers
3. Use tag components (`StaticComponent`, `ActiveComponent`) for optimization hints

### Prefab Design

1. Keep prefabs focused - one logical unit per prefab
2. Design prefab hierarchy to match expected modifications
3. Use overrides for instance-specific variations
4. Test prefab updates propagate correctly

### Serialization

1. Always include `NameComponent` for serialized entities
2. Version your custom component serialization
3. Write migration functions when changing component formats
4. Test roundtrip serialization (save/load/compare)

## Migration Guide

### Adding New Components

When adding a new serializable component:

1. Add serialization in `SceneSerializer::serialize()`
2. Add deserialization in `SceneSerializer::deserialize()`
3. Add to `Prefab::serializeEntity()` and `deserializeEntity()`
4. Update documentation

### Changing Component Format

When modifying a component's serialized format:

1. Increment `kSceneFormatVersion`
2. Add migration function in `SchemaMigration`
3. Register migration: `registerMigration(oldVersion, migrationFunc)`
4. Test with old scene files

Example migration:
```cpp
bool migrateV2ToV3(json& data) {
    for (auto& entity : data["entities"]) {
        if (entity["components"].contains("Transform")) {
            auto& t = entity["components"]["Transform"];
            // Rename "position" to "translation"
            if (t.contains("position")) {
                t["translation"] = t["position"];
                t.erase("position");
            }
        }
    }
    return true;
}
```
