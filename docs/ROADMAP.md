# Limbo Engine Roadmap (Milestones)

This roadmap assumes you already have:
- a working engine library (`engine/`) + applications (`apps/editor`, `apps/sandbox`)
- OpenGL-based 2D renderer, ECS, physics, scripting, editor, basic asset workflow, and scene serialization

The goal now is to turn “it works” into “it scales”: stability, tooling, workflows, performance, and shippable demos.

---

## How we run milestones

### Definition of Done (DoD) for every milestone
A milestone is “done” only when it has:
- **A demo** in `apps/sandbox` (or a new `apps/demo_<name>`) proving the feature
- **Docs** in `docs/` (short + practical)
- **Tests** (unit/integration) or a reproducible “verification checklist”
- **Performance + debug hooks** (stats, validation, asserts, logging)
- **No silent failures**: errors propagate to logs/UI

### Milestone rules
- Keep milestones **vertical slices** (engine feature + editor + demo), not only internal refactors.
- Every milestone ends with a **release tag** (even if “alpha”) and a short CHANGELOG entry.
- Prefer “small stable primitives” over “big frameworks”.

---

## Milestone 1 — Hardening & Developer Experience (DX)

### Outcome
You can confidently change code without breaking everything, and quickly diagnose issues.

### Deliverables
- `docs/QUALITY_BAR.md` (DoD rules, coding standards, error policy, perf budgets)
- CI: build + tests + formatting + clang-tidy (per-platform if supported)
- Crash handling + assert policy + log categories
- `apps/sandbox` adds an on-screen debug overlay (FPS, frame time, draw calls, entity count)

### Work items
- Build system
  - CMake presets (Debug/RelWithDebInfo/Release)
  - `compile_commands.json` generation by default
  - Optional: Unity builds switch, PCH switch
- Quality gates
  - clang-format check target
  - clang-tidy target with a curated rule set (start small)
  - sanitizers config (ASan/UBSan on Clang/GCC when available)
- Diagnostics
  - structured logging (subsystems + verbosity)
  - runtime “validation layer” for engine invariants (renderer, ECS, assets)
- Tests
  - first “smoke test” that boots engine headlessly (if possible)
  - tests for math utilities, asset paths, serialization roundtrips

### Acceptance checklist
- Fresh clone builds in one command
- CI is green
- A forced failure shows a clear error message and stack trace/log context

---

## Milestone 2 — Engine Core: Time, Input, and App Lifecycle

### Outcome
A consistent game loop with predictable timing, input mapping, and clean shutdown.

### Deliverables
- `docs/ENGINE_LIFECYCLE.md` (init/run/shutdown, subsystems ownership)
- Fixed timestep support (with interpolation), plus variable timestep option
- Input system with action mapping (keyboard/mouse/gamepad)

### Work items
- Time
  - fixed-update (physics) vs update (game) vs render separation
  - frame pacing controls and delta smoothing
- Input
  - action/axis mapping (JSON config)
  - editor input focus vs game input focus rules
- App lifecycle
  - subsystem init ordering & dependency graph documented
  - predictable shutdown sequence

### Acceptance checklist
- Physics runs deterministically under fixed timestep
- Inputs are remappable without recompiling
- No input conflicts between editor and viewport play mode

---

## Milestone 3 — Asset Pipeline v1 (Source → Imported → Runtime)

### Outcome
Assets are reproducible, cached, versioned, and hot-reloadable without mystery.

### Deliverables
- `docs/ASSET_PIPELINE.md`
- `tools/assetcooker` becomes the single entry point for importing
- Asset database/registry (UUID-based), import settings, and dependency tracking
- “Cooked output” directory (never manually edited)

### Work items
- Asset ID strategy
  - stable UUID for every asset
  - metadata file alongside source (or centralized registry)
- Import pipeline
  - textures → GPU-friendly format decision (at least a clean abstraction)
  - sprite sheets/atlases: build + cache + invalidation
  - audio import basics (even if limited)
- Hot reload
  - track dependencies (atlas rebuild triggers sprite updates, etc.)
  - editor feedback on reload success/failure

### Acceptance checklist
- Deleting `build/` does not break the project’s ability to rebuild assets
- Changing a texture updates the sandbox scene live (or with one reload button)
- Asset references survive renames/moves

---

## Milestone 4 — Scene/Prefab Workflow v1

### Outcome
Scenes become scalable: prefabs, overrides, hierarchy, and safe serialization.

### Deliverables
- `docs/SCENES_AND_PREFABS.md`
- Prefab assets + instance overrides (minimal but correct)
- Stronger serialization versioning and migration hooks

### Work items
- Scene graph vs ECS relationship clarified
  - parent/child transforms
  - entity lifetime rules
- Prefabs
  - authoring: create prefab from selection
  - instancing: spawn prefab, edit overrides
  - diff/merge strategy for overrides (keep it simple at first)
- Serialization
  - schema version field
  - migration registry (v1→v2) stubs

### Acceptance checklist
- A prefab edit can propagate to instances (optionally)
- Overrides survive save/load
- Backwards-compatible load works for at least one old version

---

## Milestone 5 — Editor UX: “Daily Driver” Upgrade

### Outcome
The editor is fast, stable, and supports real workflows (not only a tech demo).

### Deliverables
- `docs/EDITOR_WORKFLOWS.md` (creating scenes, prefabs, importing assets, play mode)
- Undo/Redo (command pattern)
- Gizmos: translate/rotate/scale with snapping
- Viewport play mode + pause + step frame

### Work items
- Editor architecture
  - docking layout persistence
  - panel registry (scene outliner, inspector, content browser, console)
- Undo/Redo
  - property edits, entity create/delete, component add/remove, transform changes
- Content browser
  - search, filter by type, drag-drop into scene
- Play mode
  - separate “editor world” vs “runtime world” strategy
  - safe transition (enter/exit play without corrupting editor state)

### Acceptance checklist
- You can do a full editing session without restarting
- Undo/redo works for the 10 most common actions
- Play mode doesn’t permanently mutate the scene unless explicitly applied

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
- A sample “platformer” sandbox scene behaves consistently
- Raycasts work and are visualized
- Collisions don’t spam or disappear due to ordering bugs

---

## Milestone 8 — Scripting v2 (Gameplay APIs + Hot Reload)

### Outcome
Scripting becomes a first-class gameplay layer, not just bindings.

### Deliverables
- `docs/SCRIPTING.md`
- Stable script API surface (entities, components, input, time, physics queries, events)
- Script hot reload (at least “reload on save” in editor)
- Script debugging hooks (log, error reporting, stack traces)

### Work items
- API design
  - explicit, minimal, versioned bindings
  - avoid binding “everything” early
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
