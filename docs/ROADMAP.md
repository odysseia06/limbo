# Limbo Engine Roadmap (Milestones)

This roadmap assumes you already have:
- a working engine library (`engine/`) + applications (`apps/editor`, `apps/sandbox`)
- OpenGL-based 2D renderer, ECS, physics, scripting, editor, basic asset workflow, and scene serialization

The goal now is to turn "it works" into "it scales": stability, tooling, workflows, performance, and shippable demos.

---

## Progress Overview

| Milestone | Status |
|-----------|--------|
| 1. Hardening & Developer Experience | Complete |
| 2. Engine Core: Time, Input, Lifecycle | Complete |
| 3. Asset Pipeline v1 | Complete |
| 4. Scene/Prefab Workflow v1 | Complete |
| 5. Editor UX: Daily Driver | Complete |
| 6. Rendering v2 | Pending |
| 7. Physics & Gameplay Integration | Pending |
| 8. Scripting v2 | Pending |
| 9. Performance & Scale-Up | Pending |

---

## How we run milestones

### Definition of Done (DoD) for every milestone
A milestone is "done" only when it has:
- **A demo** in `apps/sandbox` (or a new `apps/demo_<name>`) proving the feature
- **Docs** in `docs/` (short + practical)
- **Tests** (unit/integration) or a reproducible "verification checklist"
- **Performance + debug hooks** (stats, validation, asserts, logging)
- **No silent failures**: errors propagate to logs/UI

### Milestone rules
- Keep milestones **vertical slices** (engine feature + editor + demo), not only internal refactors.
- Every milestone ends with a **release tag** (even if "alpha") and a short CHANGELOG entry.
- Prefer "small stable primitives" over "big frameworks".

---

## Milestone 1 — Hardening & Developer Experience (DX) [COMPLETE]

### Outcome
You can confidently change code without breaking everything, and quickly diagnose issues.

### Delivered
- `docs/QUALITY_BAR.md` - DoD rules, coding standards, error policy
- CI: build + tests + clang-format checking
- Structured logging with categories
- Debug overlay (FPS, frame time, draw calls, entity count)
- CMake presets, compile_commands.json
- Unit tests for core systems

---

## Milestone 2 — Engine Core: Time, Input, and App Lifecycle [COMPLETE]

### Outcome
A consistent game loop with predictable timing, input mapping, and clean shutdown.

### Delivered
- `docs/ENGINE_LIFECYCLE.md` - init/run/shutdown, subsystems ownership
- Fixed timestep support with interpolation
- Input system with action mapping
- Editor/game input focus separation

---

## Milestone 3 — Asset Pipeline v1 [COMPLETE]

### Outcome
Assets are reproducible, cached, versioned, and hot-reloadable.

### Delivered
- `docs/ASSET_PIPELINE.md`
- `tools/assetcooker` for asset importing
- UUID-based asset registry with dependency tracking
- Hot-reload support with editor feedback
- Sprite atlas building and caching

---

## Milestone 4 — Scene/Prefab Workflow v1 [COMPLETE]

### Outcome
Scenes become scalable: prefabs, overrides, hierarchy, and safe serialization.

### Delivered
- `docs/SCENES_AND_PREFABS.md`
- Parent-child entity hierarchies with world transforms
- Prefab system with instance overrides
- Scene serialization with versioning
- Create prefab from selection in editor

---

## Milestone 5 — Editor UX: "Daily Driver" Upgrade [COMPLETE]

### Outcome
The editor is fast, stable, and supports real workflows.

### Delivered
- `docs/EDITOR_WORKFLOWS.md` - comprehensive workflow guide
- Undo/Redo with command pattern (10+ action types)
- Transform gizmos (translate/rotate/scale) with snapping
- Play mode with scene state preservation
- Docking layout persistence
- Content browser with search and drag-drop

---

## Milestone 6 — Rendering v2 (2D Features That Matter)

### Outcome
A renderer that supports real 2D games: materials, lighting (optional), text, and debug tools.

### Deliverables
- `docs/RENDERER2D.md`
- Material/shader system with uniform management
- Text rendering (bitmap or SDF) and UI text in editor
- Debug rendering (physics shapes, grids, bounds, picking visualizers)

### Work items
- Renderer architecture
  - resource lifetime model (GPU buffers/textures/shaders)
  - render statistics and GPU timer queries (optional)
- Batching improvements
  - sprite sorting layers, Z, transparency strategy
  - texture atlas integration
- Text
  - choose approach: bitmap first, then SDF later
- Debug
  - wireframe shapes, collider outlines, camera frustum, etc.

### Acceptance checklist
- Text works in both sandbox and editor UI
- You can visualize colliders and entity bounds
- Renderer reports draw calls + batch count + GPU/CPU frame time

---

## Milestone 7 — Physics & Gameplay Integration (Correctness First)

### Outcome
Physics feels solid: stable stepping, queries, events, and editor tooling.

### Deliverables
- `docs/PHYSICS.md`
- Physics queries (raycast, overlap) exposed to C++ and scripting
- Collision/trigger events routed through an event system
- Physics debug tools in editor

### Work items
- Stepping model
  - fixed timestep integration
  - interpolation of transforms for rendering
- ECS coupling
  - authoritative component sources (who owns transform? physics or user?)
- Queries + events
  - callbacks, queues, deterministic ordering rules

### Acceptance checklist
- A sample "platformer" sandbox scene behaves consistently
- Raycasts work and are visualized
- Collisions don't spam or disappear due to ordering bugs

---

## Milestone 8 — Scripting v2 (Gameplay APIs + Hot Reload)

### Outcome
Scripting becomes a first-class gameplay layer, not just bindings.

### Deliverables
- `docs/SCRIPTING.md`
- Stable script API surface (entities, components, input, time, physics queries, events)
- Script hot reload (at least "reload on save" in editor)
- Script debugging hooks (log, error reporting, stack traces)

### Work items
- API design
  - explicit, minimal, versioned bindings
  - avoid binding "everything" early
- Reload strategy
  - state preservation policy (what survives reload?)
- Safety
  - sandboxing rules (optional), error isolation per script instance

### Acceptance checklist
- A scripted gameplay sample exists (camera follow, shooting, triggers, UI)
- Script errors are reported in editor console with file/line
- Reload does not require restarting the editor

---

## Milestone 9 — Performance & Architecture Scale-Up

### Outcome
The engine remains fast as projects grow: streaming, jobs, and profiling.

### Deliverables
- `docs/PERFORMANCE.md`
- CPU profiler markers + GPU markers (if supported)
- Job system (minimal) and task graph for heavy subsystems
- Asset streaming/loading improvements (async IO + main-thread finalize)

### Work items
- Profiling
  - scoped timers, frame capture, CSV output
- Jobs
  - thread pool + job queue
  - clear rules: what can run off-thread
- Data-oriented fixes
  - avoid per-frame allocations (scratch allocator, arenas)
  - component iteration hotspots

### Acceptance checklist
- Profiler works in sandbox and editor
- Heavy asset load doesn't block the main thread
- Large scene (1000+ entities) stays above 60 FPS
