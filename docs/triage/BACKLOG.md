# Initial Curated Backlog

This backlog is intentionally evidence-based. Every issue seed below includes:
- labels
- concrete evidence (`file:line-range`, TODO/stub marker, or clear symptom)
- Definition of Done (DoD)

---

## [TRI-001] Add About dialog in Editor Help menu
Labels: type:feature, priority:P2-later, area:editor-tools

Evidence:
- `apps/editor/EditorApp.cpp:342-345` contains `// TODO: Show about dialog`.

Definition of Done:
- Implement a modal/dialog opened from **Help > About Limbo Editor**.
- Include app version/build info and license/credits links.
- Add an automated UI-level smoke test or scripted QA steps in docs.

---

## [TRI-002] Prevent implicit save on prefab-stage close when unsaved changes exist
Labels: type:bug, priority:P1-important, area:editor-tools

Evidence:
- `apps/editor/EditorApp.cpp:401-404` TODO for confirmation, currently closes with `m_prefabStage.close(true)`.
- `apps/editor/EditorApp.cpp:442-445` same behavior in second close path.

Definition of Done:
- Add confirmation dialog with `Save`, `Discard`, `Cancel`.
- Ensure both close paths use the same confirmation logic.
- Add regression test/QA checklist for unsaved prefab edits.

---

## [TRI-003] Implement real Save As dialog for scenes
Labels: type:bug, priority:P1-important, area:editor-tools

Evidence:
- `apps/editor/EditorApp.cpp:632-637` TODO indicates no save dialog; path is hardcoded to `assets/scenes/default.json`.

Definition of Done:
- Show Save As file picker.
- Persist chosen path and update editor state accordingly.
- Validate extension and handle overwrite confirmation.

---

## [TRI-004] Implement Asset Browser directory tree panel
Labels: type:feature, priority:P2-later, area:editor-tools

Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:106-108` has TODO and empty body for directory tree rendering.

Definition of Done:
- Render hierarchical tree on side panel.
- Support selection syncing with asset grid path.
- Handle large directory trees with lazy expansion.

---

## [TRI-005] Implement Asset Browser delete action with confirmation
Labels: type:bug, priority:P1-important, area:editor-tools, status:needs-investigation

Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:191-193` TODO for delete.

Definition of Done:
- Add delete confirmation dialog.
- Delete selected file/folder safely with error handling.
- Refresh directory view and selection state after delete.

---

## [TRI-006] Implement Asset Browser rename workflow
Labels: type:feature, priority:P2-later, area:editor-tools

Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:194-196` TODO for rename dialog.

Definition of Done:
- Provide inline or modal rename UI.
- Validate target name collisions and invalid characters.
- Preserve selection and refresh UI on success/failure.

---

## [TRI-007] Implement “Show in Explorer/Finder” action in Asset Browser
Labels: type:feature, priority:P2-later, area:editor-tools, area:platform

Evidence:
- `apps/editor/panels/AssetBrowserPanel.cpp:198-200` TODO for opening file explorer.

Definition of Done:
- Open OS file manager at selected path (or parent + selection fallback).
- Implement for Windows/Linux (and macOS if supported later).
- Show user-facing error on failure.

---

## [TRI-008] Replace sprite texture selector placeholder in Inspector
Labels: type:feature, priority:P1-important, area:editor-tools

Evidence:
- `apps/editor/panels/InspectorPanel.cpp:331-337` has `// TODO: Texture selector` and placeholder button.

Definition of Done:
- Add actual texture-picker UI integrated with asset registry.
- Apply selected texture to `SpriteRendererComponent`.
- Support clearing texture assignment.

---

## [TRI-009] Wire “Select Asset” prefab button to Asset Browser navigation
Labels: type:feature, priority:P2-later, area:editor-tools

Evidence:
- `apps/editor/panels/InspectorPanel.cpp:772-776` TODO indicates button does not select prefab in browser.

Definition of Done:
- Clicking button focuses Asset Browser and highlights target prefab asset.
- Handle missing/invalid prefab IDs gracefully.
- Add QA scenario for prefab instance inspection flow.

---

## [TRI-010] Assign dropped texture when creating sprite from viewport drag-drop
Labels: type:bug, priority:P1-important, area:editor-tools, area:render-2d

Evidence:
- `apps/editor/panels/ViewportPanel.cpp:205-209` TODO indicates texture is not assigned after sprite creation.

Definition of Done:
- On valid texture drop, create entity and assign texture asset ID/reference.
- Reject unsupported asset types with clear feedback.
- Add a drag-drop integration test or deterministic QA script.

---

## [TRI-011] Move editor viewport rendering to framebuffer path
Labels: type:tech-debt, priority:P1-important, area:editor-tools, area:renderer

Evidence:
- `apps/editor/panels/ViewportPanel.cpp:240-245` TODO says rendering is currently direct-to-screen.

Definition of Done:
- Render scene into framebuffer texture used by viewport panel.
- Handle viewport resize and pixel ratio updates.
- Keep scene camera and picking logic consistent after migration.

---

## [TRI-012] Complete “Apply All” in Prefab Overrides panel
Labels: type:bug, priority:P1-important, area:editor-tools, area:ecs-scene

Evidence:
- `apps/editor/panels/PrefabOverridesPanel.cpp:163-167` TODO and log state "not yet fully implemented".

Definition of Done:
- Apply accumulated overrides back to prefab asset.
- Persist changes and refresh all relevant instances.
- Add revert/undo safety and QA coverage.

---

## [TRI-013] Support AddComponent/RemoveComponent prefab overrides
Labels: type:tech-debt, priority:P1-important, area:ecs-scene

Evidence:
- `engine/src/scene/Prefab.cpp:665-669` TODO notes only property overrides are supported.

Definition of Done:
- Extend override apply logic to handle add/remove component kinds.
- Keep schema serialization/deserialization compatible.
- Add tests covering add/remove override round-trip.

---

## [TRI-014] Expand Prefab::updateInstances beyond Transform-only synchronization
Labels: type:bug, priority:P1-important, area:ecs-scene

Evidence:
- `engine/src/scene/Prefab.cpp:580-604` comment explicitly says update is simplified and only Transform is handled.

Definition of Done:
- Update all supported component types while respecting override masks.
- Ensure deterministic update ordering for nested prefab instances.
- Add tests for at least SpriteRenderer, Script, and collider components.

---

## [TRI-015] Implement asset enumeration in ImGui debug Asset Browser panel
Labels: type:feature, priority:P2-later, area:editor-tools, area:asset-pipeline

Evidence:
- `engine/src/imgui/DebugPanels.cpp:194-196` says asset list is not implemented.

Definition of Done:
- Provide API to iterate currently loaded assets and show basic metadata.
- Add filter/search in debug panel.
- Keep panel performant with large asset sets.

---

## [TRI-016] Replace UI Label placeholder rendering with real text drawing
Labels: type:tech-debt, priority:P2-later, area:ui, area:render-2d

Evidence:
- `engine/include/limbo/ui/Widgets.hpp:26-30` documents Label as placeholder only.
- `engine/src/ui/Widgets.cpp:28-40` draws a colored bar instead of text glyphs.

Definition of Done:
- Render actual text for `Label` using font atlas/glyph pipeline.
- Support alignment, color, clipping, and baseline behavior.
- Add UI rendering tests or screenshot-based regression checks.

---

## [TRI-017] Improve thick-circle rendering in Renderer2D
Labels: type:perf, priority:P2-later, area:render-2d, area:renderer

Evidence:
- `engine/src/render/2d/Renderer2D.cpp:696-724` comments indicate ring rendering is approximated by inner/outer line segments.

Definition of Done:
- Replace approximation with proper ring geometry path.
- Preserve batching where possible and compare draw-call impact.
- Add visual regression scene for circle quality.

---

## [TRI-018] Surface text input events from platform layer instead of dropping them
Labels: type:bug, priority:P1-important, area:input, area:platform

Evidence:
- `engine/src/platform/Input.cpp:312-315` char callback intentionally ignores codepoints.

Definition of Done:
- Capture UTF codepoints into a frame input buffer.
- Expose API for UI/text fields to consume typed characters.
- Add tests for printable chars and basic IME-safe behavior assumptions.

---

## [TRI-019] Upgrade texture import path beyond raw file copy
Labels: type:tech-debt, priority:P2-later, area:asset-pipeline

Evidence:
- `engine/src/assets/AssetImporter.cpp:20-37` states textures are directly copied without preprocessing.

Definition of Done:
- Implement configurable import steps (e.g., mipmap generation, format conversion metadata).
- Persist import settings and deterministic outputs.
- Add asset pipeline tests for texture import reproducibility.

---

## [TRI-020] Add shader validation/precompile stage in ShaderImporter
Labels: type:tech-debt, priority:P2-later, area:asset-pipeline, area:renderer

Evidence:
- `engine/src/assets/AssetImporter.cpp:75-80` comments say shader importer currently just copies source.

Definition of Done:
- Validate shader source during import.
- Optionally precompile/cache intermediate format.
- Emit actionable diagnostics for failed imports.

---

## [TRI-021] Add audio conversion/metadata extraction in AudioImporter
Labels: type:tech-debt, priority:P2-later, area:asset-pipeline, area:audio

Evidence:
- `engine/src/assets/AssetImporter.cpp:113-118` indicates audio importer only copies files.

Definition of Done:
- Normalize/convert formats as configured.
- Extract and store metadata (duration, sample rate, channels).
- Add importer tests for supported audio formats.

---

## [TRI-022] Resolve exposed Renderer3D API without implementation
Labels: type:feature, priority:P2-later, area:render-3d, status:blocked

Evidence:
- `engine/include/limbo/render/3d/Renderer3D.hpp:5-9` marks API as not yet implemented.
- `engine/src/render/3d` directory is present but empty (clear symptom).

Definition of Done:
- Choose and execute one path:
  - implement minimal Renderer3D runtime, or
  - gate/remove public API from stable header surface.
- Align docs and examples with actual support level.
- Add initial 3D smoke test if implementation is enabled.

---

## [TRI-023] Resolve placeholder 3D data APIs (Mesh/Material/Model/Lighting)
Labels: type:tech-debt, priority:P2-later, area:render-3d, status:blocked

Evidence:
- `engine/include/limbo/render/3d/Mesh.hpp:5-9` placeholder marker.
- `engine/include/limbo/render/3d/Material.hpp:5-9` placeholder marker.
- `engine/include/limbo/render/3d/Model.hpp:5-9` placeholder marker.
- `engine/include/limbo/render/3d/Lighting.hpp:5-9` placeholder marker.
- `engine/src/render/3d` has no backing source implementation (clear symptom).

Definition of Done:
- Decide staged scope for each type (MVP or hidden).
- Ensure every exposed type has either implementation or explicit compile-time guard.
- Add compile/test coverage for whichever path is chosen.

---

## [TRI-024] Implement or gate MeshRenderSystem 3D ECS integration
Labels: type:tech-debt, priority:P2-later, area:render-3d, area:ecs-scene, status:blocked

Evidence:
- `engine/include/limbo/ecs/systems/3d/MeshRenderSystem.hpp:9-27` declares runtime API.
- `engine/src/ecs/systems` currently only contains `2d/SpriteRenderSystem.cpp` and `2d/TextRenderSystem.cpp` (clear symptom).

Definition of Done:
- Implement MeshRenderSystem cpp path and registration, or remove/gate API.
- Ensure system lifecycle hooks are covered by tests.
- Document 3D ECS render support status.

---

## [TRI-025] Resolve Physics3D skeleton API with no source implementation
Labels: type:feature, priority:P2-later, area:physics, status:blocked

Evidence:
- `engine/include/limbo/physics/3d/Physics3D.hpp:17` notes skeleton/future implementation.
- `engine/src/physics/3d` directory is empty (clear symptom).

Definition of Done:
- Select backend/runtime plan (or hide API until ready).
- Implement minimal stepping + gravity + one query, or remove public exposure.
- Add baseline tests for enabled behavior.

---

## [TRI-026] Resolve PhysicsSystem3D skeleton API with no source implementation
Labels: type:tech-debt, priority:P2-later, area:physics, area:ecs-scene, status:blocked

Evidence:
- `engine/include/limbo/physics/3d/PhysicsSystem3D.hpp:14` notes skeleton/future implementation.
- `engine/src/physics/3d` directory is empty (clear symptom).

Definition of Done:
- Implement `PhysicsSystem3D` lifecycle + sync paths or gate/remove public header.
- Ensure component/runtime pointer ownership model is defined.
- Add ECS integration tests when enabled.

---

## [TRI-027] Expand clang-tidy CI scope to full codebase (or deterministic shard set)
Labels: type:tech-debt, priority:P1-important, area:build-ci

Evidence:
- `.github/workflows/ci.yml:101-102` runs clang-tidy on `find ... | head -50`, leaving most files unchecked.

Definition of Done:
- Replace fixed `head -50` truncation with full scan or deterministic sharded scans.
- Keep runtime acceptable via caching and/or matrix split.
- Report analyzed file count in CI logs.

