# Curated Backlog

This backlog is evidence-backed and intended for direct issue creation.
Parsing contract for automation:
- Each issue starts with `### ISSUE-XXX: <Title>`.
- Next line contains `Labels: ...` as comma-separated labels.
- Body includes Evidence and Definition of Done.

## Backlog Items

### ISSUE-001: Editor: Add About dialog in Help menu
Labels: type:feature, priority:P2-later, area:editor-tools
Evidence:
- `apps/editor/EditorApp.cpp:342-345` contains `// TODO: Show about dialog` in Help menu action.
Definition of Done:
- Implement About modal with engine/app version and basic links.
- Add editor test/manual checklist entry covering menu action and close behavior.

### ISSUE-002: Editor: Confirm unsaved prefab changes when returning to scene via breadcrumb
Labels: type:bug, priority:P1-important, area:editor-tools
Evidence:
- `apps/editor/EditorApp.cpp:396-405` uses default save behavior with TODO for confirmation dialog.
Definition of Done:
- Show explicit Save/Discard/Cancel dialog when prefab stage is dirty.
- Ensure Cancel keeps prefab stage open and does not modify asset.

### ISSUE-003: Editor: Confirm unsaved prefab changes when closing prefab stage
Labels: type:bug, priority:P1-important, area:editor-tools
Evidence:
- `apps/editor/EditorApp.cpp:440-445` closes prefab with default save path and TODO for confirmation.
Definition of Done:
- Reuse consistent Save/Discard/Cancel flow for Close button.
- Add regression test or scripted manual repro for dirty prefab close path.

### ISSUE-004: Editor: Replace hardcoded Save As path with file picker
Labels: type:feature, priority:P1-important, area:editor-tools
Evidence:
- `apps/editor/EditorApp.cpp:632-637` has TODO and currently writes to `assets/scenes/default.json`.
Definition of Done:
- Add native or ImGui-based file picker for scene save target.
- Preserve last-used directory and avoid silent overwrite without user intent.

### ISSUE-005: Viewport: Assign dropped texture assets to SpriteRenderer
Labels: type:bug, priority:P1-important, area:editor-tools, area:renderer
Evidence:
- `apps/editor/panels/ViewportPanel.cpp:196-209` creates sprite entity but TODO leaves texture unassigned.
Definition of Done:
- Drag-drop image assigns texture asset reference on `SpriteRendererComponent`.
- New entity renders with the dropped texture in viewport.

### ISSUE-006: Viewport: Render through framebuffer and present as image
Labels: type:feature, priority:P1-important, area:editor-tools, area:renderer
Evidence:
- `apps/editor/panels/ViewportPanel.cpp:240-243` includes TODO and currently renders directly to screen.
Definition of Done:
- Render scene to dedicated framebuffer and display texture in viewport panel.
- Support resize handling and correct picking coordinates in framebuffer space.

### ISSUE-007: Asset Browser: Implement directory tree side panel
Labels: type:feature, priority:P2-later, area:editor-tools, area:asset-pipeline
Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:106-108` contains TODO for directory tree view.
Definition of Done:
- Add hierarchical folder tree with expand/collapse and selection sync.
- Selecting tree node updates asset grid path and breadcrumbs.

### ISSUE-008: Asset Browser: Implement Delete action with confirmation
Labels: type:feature, priority:P1-important, area:editor-tools, area:asset-pipeline
Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:191-193` contains TODO for delete flow.
Definition of Done:
- Add confirmation dialog and delete file/folder from disk safely.
- Refresh browser state and handle error reporting for failed deletions.

### ISSUE-009: Asset Browser: Implement Rename action
Labels: type:feature, priority:P1-important, area:editor-tools, area:asset-pipeline
Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:194-196` contains TODO for rename dialog.
Definition of Done:
- Implement inline or modal rename with validation and conflict checks.
- Persist rename to filesystem and refresh related UI references.

### ISSUE-010: Asset Browser: Implement Show in Explorer/Finder action
Labels: type:feature, priority:P2-later, area:editor-tools, area:platform
Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:198-200` contains TODO for opening file explorer.
Definition of Done:
- Implement cross-platform open-in-file-manager for selected asset/folder.
- Gracefully handle unsupported platform or command failure.

### ISSUE-011: Inspector: Implement SpriteRenderer texture selector
Labels: type:feature, priority:P1-important, area:editor-tools, area:renderer
Evidence:
- `apps/editor/panels/InspectorPanel.cpp:331-338` shows TODO and placeholder button `None (Select)`.
Definition of Done:
- Add texture picker integrated with asset browser/registry.
- Persist selected texture on component and render immediate preview.

### ISSUE-012: Inspector: Wire "Select Asset" for prefab instances
Labels: type:feature, priority:P2-later, area:editor-tools, area:asset-pipeline
Evidence:
- `apps/editor/panels/InspectorPanel.cpp:772-776` logs click and TODO for selecting prefab in browser.
Definition of Done:
- Clicking `Select Asset` focuses asset browser and highlights prefab asset.
- Handle missing prefab asset path with non-fatal user feedback.

### ISSUE-013: Prefab Overrides panel: Implement Apply All to prefab asset
Labels: type:feature, priority:P1-important, area:editor-tools, area:ecs-scene
Evidence:
- `apps/editor/panels/PrefabOverridesPanel.cpp:163-167` logs "not yet fully implemented" and TODO.
Definition of Done:
- Apply all root-instance overrides back to prefab file with save confirmation.
- Emit clear success/failure result in UI and keep undo/dirty state consistent.

### ISSUE-014: Prefab override engine: Support AddComponent/RemoveComponent override kinds
Labels: type:feature, priority:P1-important, area:ecs-scene
Evidence:
- `engine/src/scene/Prefab.cpp:665-669` TODO says only property overrides are currently supported.
Definition of Done:
- Support AddComponent and RemoveComponent in apply/revert/update flows.
- Add tests covering override roundtrip for add/remove component operations.

### ISSUE-015: Scene module: Replace placeholder Scene class implementation
Labels: type:tech-debt, priority:P1-important, area:ecs-scene
Evidence:
- `engine/include/limbo/scene/Scene.hpp:8-15` marks Scene as placeholder.
- `engine/src/scene/Scene.cpp:5-7` update function is placeholder no-op.
Definition of Done:
- Define concrete Scene responsibilities and implement non-noop update lifecycle.
- Add tests verifying scene-level behavior expected by editor/runtime.

### ISSUE-016: Debug panels: Implement asset list in Asset Browser debug window
Labels: type:feature, priority:P2-later, area:editor-tools, area:asset-pipeline
Evidence:
- `engine/src/imgui/DebugPanels.cpp:194-197` prints `(Asset list not yet implemented)`.
Definition of Done:
- Show loaded assets and key metadata in debug asset browser.
- Add filtering/sorting basics to keep panel usable for large projects.

### ISSUE-017: UI widgets: Implement text rendering for Label widget
Labels: type:feature, priority:P2-later, area:ui, area:renderer
Evidence:
- `engine/include/limbo/ui/Widgets.hpp:26-30` documents Label as placeholder rectangle without text rendering.
Definition of Done:
- Render label text using engine font/text pipeline.
- Preserve existing style properties (color, alignment, clipping) for labels.

### ISSUE-018: Public API hygiene: Gate skeleton 3D/physics headers in `limbo/Limbo.hpp`
Labels: type:tech-debt, priority:P1-important, area:core-runtime, status:blocked
Evidence:
- `engine/include/limbo/Limbo.hpp:31-37` exports 3D renderer skeleton headers.
- `engine/include/limbo/Limbo.hpp:67-70` exports physics 3D skeleton headers.
Definition of Done:
- Either hide skeleton APIs behind opt-in macro/namespace or provide complete implementations.
- Document stability level of exported subsystems in API docs.

### ISSUE-019: ECS 3D: Implement `MeshRenderSystem` source and build integration
Labels: type:feature, priority:P1-important, area:ecs-scene, area:renderer, status:blocked
Evidence:
- `engine/include/limbo/ecs/systems/3d/MeshRenderSystem.hpp:14-31` declares API.
- `engine/CMakeLists.txt:72-79` lists only 2D ECS render system sources.
Definition of Done:
- Add `engine/src/ecs/systems/3d/MeshRenderSystem.cpp` with attach/detach/update/render.
- Register/compile source in CMake and add basic render-path test coverage.

### ISSUE-020: Physics 3D: Implement `Physics3D` and `PhysicsSystem3D` runtime
Labels: type:feature, priority:P1-important, area:physics, status:blocked
Evidence:
- `engine/include/limbo/physics/3d/Physics3D.hpp:17-19` marks class as future skeleton.
- `engine/include/limbo/physics/3d/PhysicsSystem3D.hpp:14-16` marks system as future skeleton.
- `engine/CMakeLists.txt:86-90` includes only physics 2D sources.
Definition of Done:
- Provide concrete 3D physics backend integration and system lifecycle.
- Add unit/integration tests for step, body sync, and raycast queries.

### ISSUE-021: Renderer 3D: Implement `Renderer3D` backend and source files
Labels: type:feature, priority:P1-important, area:renderer, status:blocked
Evidence:
- `engine/include/limbo/render/3d/Renderer3D.hpp:5-9` says API is not yet implemented.
- `engine/CMakeLists.txt:44-51` includes render 2D sources but no render 3D sources.
Definition of Done:
- Add renderer 3D source implementation with init/begin/submit/end path.
- Add draw statistics validation and at least one smoke test/example scene.

### ISSUE-022: 3D assets: Implement `Mesh`/`Model`/`Material`/`Lighting` runtime paths
Labels: type:feature, priority:P2-later, area:renderer, area:asset-pipeline, status:blocked
Evidence:
- `engine/include/limbo/render/3d/Mesh.hpp:5-9` placeholder note.
- `engine/include/limbo/render/3d/Model.hpp:5-9` placeholder note.
- `engine/include/limbo/render/3d/Material.hpp:5-9` placeholder note.
- `engine/include/limbo/render/3d/Lighting.hpp:5-9` placeholder note.
Definition of Done:
- Implement core runtime behavior for declared types and remove placeholder caveats.
- Add loader/material/light tests covering at least one model render path.

### ISSUE-023: CI: Remove `head -50` cap from clang-tidy workflow
Labels: type:tech-debt, priority:P1-important, area:build-ci
Evidence:
- `.github/workflows/ci.yml:124-126` runs clang-tidy on only first 50 cpp files.
Definition of Done:
- Analyze full intended source set (or deterministic allowlist) without silent truncation.
- Keep CI runtime acceptable with documented scope partitioning if needed.

### ISSUE-024: CI: Fix format-check `find` expression precedence
Labels: type:bug, priority:P1-important, area:build-ci, platform:linux
Evidence:
- `.github/workflows/ci.yml:92-93` uses `find engine apps tests -name "*.cpp" -o -name "*.hpp"` without grouping.
Definition of Done:
- Use grouped predicates so only desired roots/extensions are checked.
- Confirm no false positives/negatives by running formatter check locally and in CI.

### ISSUE-025: AssetLoader: Avoid main-thread full-load fallback stalls
Labels: type:perf, priority:P1-important, area:asset-pipeline, area:core-runtime
Evidence:
- `engine/src/assets/AssetLoader.cpp:71-74` performs full `asset->load()` on main thread when async split unsupported.
- `engine/src/assets/AssetLoader.cpp:121-127` flags fallback path in IO worker stage.
Definition of Done:
- Add non-blocking strategy for non-async assets (chunking, preload, or explicit sync queue budget).
- Add profiling metric that caps per-frame asset upload/load time.

### ISSUE-026: HotReload: Make cycle fallback deterministic and observable
Labels: type:tech-debt, priority:P2-later, area:asset-pipeline, status:needs-investigation
Evidence:
- `engine/src/assets/HotReloadManager.cpp:375-383` falls back to arbitrary order on dependency cycle.
Definition of Done:
- Replace arbitrary fallback with deterministic ordering and explicit cycle reporting.
- Add tests for cycle detection behavior and expected reload order output.

### ISSUE-027: HotReload: Avoid duplicate immediate reload passes for multi-asset file changes
Labels: type:perf, priority:P2-later, area:asset-pipeline, status:needs-investigation
Evidence:
- `engine/src/assets/HotReloadManager.cpp:285-287` calls `triggerReload` for each watcher on changed path.
- `engine/src/assets/HotReloadManager.cpp:194-197` immediate processing occurs when batching is disabled.
Definition of Done:
- Coalesce same-file change events into one processing pass when possible.
- Add benchmark/log metric showing reduced redundant reload processing.

### ISSUE-028: Renderer buffer validation: Replace unknown-type assert crash path
Labels: type:bug, priority:P2-later, area:renderer, area:core-runtime
Evidence:
- `engine/src/render/common/Buffer.cpp:150-151` asserts on unknown shader data type.
- `engine/src/render/common/Buffer.cpp:248-250` repeats assert in vertex attribute setup.
Definition of Done:
- Convert fatal assert path to guarded validation with actionable error message.
- Add tests for invalid layout handling without process abort in non-debug builds.

