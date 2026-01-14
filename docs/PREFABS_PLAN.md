# Prefabs Plan (Unity-style with a bit of Godot practicality) — Limbo

This document describes a **Unity-style prefab system** for Limbo (ECS + editor), borrowing a few pragmatic UX ideas from **Godot’s “scenes as reusable node trees”** (instancing + hiding internals by default).

**Goals**
- Make prefabs feel like Unity: **Prefab Mode**, **Overrides (Apply/Revert)**, **Unpack**, later **Nested Prefabs** and **Prefab Variants**.
- Keep implementation **shippable in phases** (Prefab v1 → v1.5 → v2 → v3).
- Preserve your engine architecture: **visual and physics components are separate**, prefabs are **content workflow** on top.

---

## 1) Target UX (Unity-first)

### 1.1 Editor workflow
1. **Create Prefab** from selected entities.
2. **Drag prefab** from Content Browser into scene → creates an instance.
3. Selecting a prefab instance root shows:
   - **Open Prefab** (Prefab Mode)
   - **Overrides (N)** panel with per-override **Apply** / **Revert** and **Apply All** / **Revert All**
   - **Unpack** and **Unpack Completely** (remove prefab link, keep current state)
4. Prefab Mode editing:
   - Edit prefab **in isolation** (in-context view later)

### 1.2 Runtime expectations
- Instantiation is deterministic and fast.
- Overrides apply consistently.
- Prefab references are stable across renames/moves.

---

## 2) Core architecture decisions (must-do)

### A) Stable IDs are non-negotiable
You need **two kinds of IDs**:

1) **Prefab-local ID** (`local_id`)
- Stable *within the prefab asset*.
- Used to target entities for overrides.

2) **Scene GUID** (`entity_guid`)
- Stable *within a scene* and across saves.
- Used for external references (scripts, links, etc.).

> Internally you still use `entt::entity`, but it’s not serialized.

### B) Prefab instances must remember origin
Every entity spawned from a prefab needs a link back to the prefab:

- `PrefabInstanceComponent` on the **instance root** (recommended)
- `PrefabLinkComponent` on **each entity** in the instance, storing:
  - `prefab_asset_id`
  - `instance_id`
  - `local_id` (prefab-local entity id)

This enables:
- overrides UI
- apply/revert
- updating instances when prefab asset changes

### C) Prefer “override records” over “saving whole instance blobs”
Store **diffs** (overrides) rather than full snapshots. This scales and matches Unity’s mental model.

---

## 3) Data model

### 3.1 Prefab asset file format
Store a **tree** of entities with components.

Example: `assets/prefabs/Crate.prefab.json`

```json
{
  "type": "PrefabAsset",
  "version": 1,
  "prefab_id": "a6d0b5c9-2b2d-4a0f-9f5d-3cde3a9fe2aa",
  "root_local_id": "root",
  "entities": [
    {
      "local_id": "root",
      "name": "Crate",
      "parent_local_id": null,
      "components": {
        "Transform": { "pos": [0,0], "rot": 0, "scale": [1,1] },
        "QuadRenderer": { "size": [1,1], "color": [1,1,1,1] },
        "BoxCollider2D": { "size": [1,1], "offset": [0,0], "isTrigger": false }
      }
    }
  ]
}
```

**Notes**
- `local_id` is the primary key.
- Parent relation uses `parent_local_id` (not indices).
- Components are serialized by type name or type id.

### 3.2 Prefab instance representation in a scene
In scene JSON, store:
- normal entities
- a `prefab_instances` list with per-instance overrides

Example snippet inside `scene.json`:

```json
{
  "prefab_instances": [
    {
      "instance_id": "d4a9d8b2-1f5f-4b23-8ed0-1d38d4a2e9cc",
      "prefab_id": "a6d0b5c9-2b2d-4a0f-9f5d-3cde3a9fe2aa",
      "root_entity_guid": "3f21...",

      "overrides": [
        {
          "kind": "Property",
          "target_local_id": "root",
          "component": "Transform",
          "property": "pos",
          "value": [10, 2]
        },
        {
          "kind": "Property",
          "target_local_id": "root",
          "component": "QuadRenderer",
          "property": "color",
          "value": [1, 0.7, 0.7, 1]
        }
      ]
    }
  ]
}
```

---

## 4) Override types (implement in increasing difficulty)

### Phase 1: Property overrides (easy, high value)
- When an instance modifies a component field, record a `Property` override.
- Inspector shows overrides with **Revert** or **Apply**.

Schema:
- `kind: Property`
- `target_local_id`
- `component`
- `property` (string path like `size.x` or `pos`)
- `value` (JSON)

### Phase 2: Added/Removed components (medium)
Unity treats these as overrides too.

Add:
- `kind: AddComponent`
- `kind: RemoveComponent`

### Phase 3: Added/Removed entities (hard but important)
Allows workflows like “add a child marker to one instance”.

Add:
- `kind: AddEntity` (with generated `local_id` unique within *instance*)
- `kind: RemoveEntity`

Practical trick:
- Instance-only entities use reserved ids like: `__added__/guid`.
- They are not part of the prefab asset until user **Apply**s them.

---

## 5) Instance update rules (the “prefabs feel real” part)

When the prefab asset changes, instances update via:

1. Rebuild instance from prefab asset **baseline**
2. Reapply stored overrides
3. Preserve instance-added entities/components as overrides (later phases)

This yields:
- prefab edits propagate to instances automatically
- instance changes remain as overrides until applied/reverted

---

## 6) Prefab Mode (Edit Prefab Asset) — Unity style

### 6.1 Implement isolation first (recommended)
- Open a separate “Prefab Stage World”
- Load prefab asset graph into that world
- User edits it like a scene
- Save writes back to `.prefab.json`
- On save: open scenes update instances of this prefab
  - rebuild baseline + reapply overrides

### 6.2 Deliverables
- `apps/editor/PrefabStage.hpp/.cpp`
- Inspector button: **Open Prefab**
- Breadcrumb UI: `Scene > Prefab: Crate`
- Save / Discard changes

---

## 7) Overrides UI (Apply / Revert) — Unity mental model

Selecting a prefab instance root shows an **Overrides (N)** panel listing changes like:
- `Transform.pos`
- `QuadRenderer.color`
- `BoxCollider2D.size`

Each override item has:
- **Revert** (remove override, reset to prefab baseline)
- **Apply** (write change into prefab asset, remove override)

Also provide:
- **Apply All**
- **Revert All**

> Phase 1 can limit Apply to “this prefab asset”. Variants later add apply-target selection.

---

## 8) Unpack / Unpack Completely

### Unpack
- Remove prefab link components (`PrefabInstanceComponent` / `PrefabLinkComponent`)
- Keep current component values (including overrides)

### Unpack Completely
- If nested prefabs exist, also unpack nested instances.

This is the “escape hatch” when prefab structure is no longer desired.

---

## 9) Prefab Variants (Prefab v3)

A prefab variant is a prefab asset that references a base prefab and adds overrides.

Example: `EnemyVariant.prefab.json`

```json
{
  "type": "PrefabVariant",
  "version": 1,
  "prefab_id": "...",
  "base_prefab_id": "...",
  "variant_overrides": [
    { "kind": "Property", "target_local_id": "root", "component": "QuadRenderer", "property": "color", "value": [1,0,0,1] }
  ]
}
```

Resolution:
1. Load base prefab
2. Apply variant overrides → produce “variant prefab graph”
3. Instances reference the **variant prefab_id**

---

## 10) Nested prefabs (Prefab v2)

Support prefab instances inside other prefabs while retaining links.

Represent nested instance in prefab asset via a component like:

```json
"components": {
  "PrefabChildInstance": {
    "child_prefab_id": "...",
    "child_instance_id": "..."
  }
}
```

During prefab load:
- instantiate the child prefab into parent graph
- keep linkage so editing child updates everywhere

**Important**: multi-level apply rules can get tricky; initially restrict Apply to current level or warn.

---

## 11) Godot ideas worth stealing (even if Unity-style)

Godot’s instanced scenes behave like a single node with internals hidden by default.
For Limbo, that maps to:
- show prefab instance as a single root in hierarchy by default
- allow “expand internals” optionally later
- keep defaults clean for level building

---

## 12) Implementation plan (phased, shippable)

### Prefabs v1 (2–3 weeks)
**Scope**
- Prefab Asset
- Instantiate into scene
- Property overrides only
- Overrides UI (Apply/Revert)
- Prefab Mode (isolation)
- Unpack

**Acceptance**
- Create prefab → drag into scene → edit instance → overrides panel shows diffs → apply/revert works
- Editing prefab asset updates instances (baseline rebuild + override reapply)
- Unpack removes link and preserves current state

### Prefabs v1.5
**Scope**
- Add/Remove component overrides
- More stable diffing & override pruning (if value matches baseline, drop override)
- Better hierarchy UX (prefab badge, expand placeholder)

### Prefabs v2
**Scope**
- Nested prefabs
- Added/Removed entity overrides
- Safer apply rules across levels

### Prefabs v3
**Scope**
- Prefab variants
- Apply target selection (apply to base vs variant)

---

## 13) Suggested module/file layout

```
engine/include/limbo/prefab/
  PrefabAsset.hpp
  PrefabVariant.hpp
  PrefabGraph.hpp
  PrefabInstance.hpp
  PrefabOverrides.hpp
  PrefabSystem.hpp

engine/src/prefab/
  PrefabSystem.cpp
  PrefabSerializer.cpp
  PrefabInstantiator.cpp
  PrefabOverrideApplier.cpp
  PrefabDiff.cpp

apps/editor/
  PrefabStage.cpp
  panels/PrefabOverridesPanel.cpp
```

---

## 14) Two key algorithms you’ll want early

### 14.1 PrefabDiff (instance vs prefab baseline)
Inputs:
- prefab baseline component data
- instance component data

Output:
- minimal list of overrides (diffs)

Used by:
- Overrides UI
- Revert All / Apply All
- Override pruning when instance matches prefab again

### 14.2 PrefabRebuild (on prefab asset change)
On prefab save:
1) rebuild instance entities from prefab baseline
2) reapply overrides
3) preserve instance-only additions (later)

This is the core of “prefab edits propagate”.

---

## 15) Practical constraints (recommended defaults)
- Prefer collider shapes authoritative; “Fit to Visual” is a copy operation (unless user enables auto-fit).
- Physics callbacks must enqueue changes; apply entity destruction via deferred command buffer.
- Scene and prefab formats must include version fields from day one.

---

## Next step to tailor to your repo
To plug directly into Limbo, adapt:
- component names
- hierarchy representation
- current scene JSON schema
- asset registry IDs

If you share your current `SceneSerializer` structure and entity hierarchy component, this plan can be turned into exact file-level tasks.
