# Curated Backlog

This backlog is evidence-based. Every issue includes:
- **Labels**: exactly one `type:*`, one `priority:*`, and 1–3 `area:*` labels
- **Context**: why the issue matters
- **Evidence**: `file:line-range` with TODO/stub marker or symptom
- **Approach Hints**: 2–5 bullets on how to implement
- **Definition of Done** (DoD): concrete acceptance criteria

Total issues: **20**

---

## [TRI-001] Add About dialog in Editor Help menu
Labels: type:feature, priority:P2-later, area:editor-tools

**Context:**
The Help menu has an About item that does nothing. Low priority but trivial to implement
and good polish for the editor.

**Evidence:**
- `apps/editor/EditorApp.cpp:342-345` — `// TODO: Show about dialog` inside empty menu item handler.

**Approach Hints:**
- Create an ImGui modal window triggered from the menu item.
- Display version string from a `Version.hpp` or CMake-generated define.
- Include a link/text for license info and third-party credits.

**Definition of Done:**
- [ ] About modal opens from Help > About and displays version + credits.
- [ ] Modal closes cleanly with a button or Escape.
- [ ] No hardcoded version string — derive from build system.

---

## [TRI-002] Add confirmation dialog when closing prefab stage with unsaved changes
Labels: type:bug, priority:P1-important, area:editor-tools

**Context:**
Closing a prefab stage with unsaved changes auto-saves without asking. This is a data-loss
risk — the user may have been experimenting and wants to discard changes.

**Evidence:**
- `apps/editor/EditorApp.cpp:401-404` — `// TODO: Show confirmation dialog`, calls `m_prefabStage.close(true)`.
- `apps/editor/EditorApp.cpp:442-445` — identical pattern in second close path.

**Approach Hints:**
- Introduce a shared confirmation dialog helper (Save / Discard / Cancel) in the editor UI layer.
- Call the helper from both close paths before `m_prefabStage.close()`.
- On Cancel, abort the close. On Discard, call `close(false)`. On Save, call `close(true)`.
- Re-use this helper later for scene close and editor quit flows.

**Definition of Done:**
- [ ] Both prefab-stage close paths show Save / Discard / Cancel dialog.
- [ ] Discard path does not persist unsaved changes.
- [ ] Cancel path aborts the close entirely.
- [ ] Manual QA scenario documented for unsaved-prefab-close flow.

---

## [TRI-003] Implement Save As dialog for scenes
Labels: type:feature, priority:P1-important, area:editor-tools

**Context:**
"Save As" currently hardcodes the output path to `assets/scenes/default.json`. Users cannot
choose a file name or location, making multi-scene workflows impossible and risking accidental
overwrites.

**Evidence:**
- `apps/editor/EditorApp.cpp:632-637` — `// TODO: Save file dialog`, path hardcoded.

**Approach Hints:**
- Use a platform file dialog (e.g., tinyfd, nfd, or OS-native via GLFW extension).
- Validate chosen extension (`.json`), prompt for overwrite if file exists.
- After successful save, update the editor title bar and internal scene path.
- Fall back to a simple ImGui text-input dialog if native dialogs are not available.

**Definition of Done:**
- [ ] Save As opens a file picker or input dialog.
- [ ] Chosen path is persisted and used for subsequent Save operations.
- [ ] Overwrite confirmation shown when target file exists.
- [ ] Editor title bar reflects the new scene path.

---

## [TRI-004] Implement Asset Browser directory tree panel
Labels: type:feature, priority:P2-later, area:editor-tools

**Context:**
The Asset Browser has a stub for a directory tree side panel. Without it, navigating large
asset hierarchies requires clicking through breadcrumbs one level at a time.

**Evidence:**
- `apps/editor/panels/AssetBrowserPanel.cpp:106-108` — empty body for `drawDirectoryTree()`.

**Approach Hints:**
- Use `ImGui::TreeNodeEx` with `ImGuiTreeNodeFlags_OpenOnArrow` for lazy expansion.
- Sync selection state bidirectionally with the asset grid path.
- Only scan child directories on expansion to handle large trees.
- Highlight the currently browsed directory in the tree.

**Definition of Done:**
- [ ] Directory tree renders in a side panel next to the asset grid.
- [ ] Clicking a tree node navigates the grid to that directory.
- [ ] Tree handles directories with 100+ children without visible lag.

---

## [TRI-005] Implement Asset Browser context-menu actions (Delete, Rename, Show in Explorer)
Labels: type:feature, priority:P2-later, area:editor-tools, area:platform

**Context:**
The Asset Browser right-click menu has three stub items: Delete, Rename, and Show in Explorer.
These are standard file management actions expected in any asset browser. Consolidated from
three separate stubs because they share the same context-menu code path and can be implemented
together.

**Evidence:**
- `apps/editor/panels/AssetBrowserPanel.cpp:191-193` — `// TODO: Confirm and delete`.
- `apps/editor/panels/AssetBrowserPanel.cpp:194-196` — `// TODO: Rename dialog`.
- `apps/editor/panels/AssetBrowserPanel.cpp:198-200` — `// TODO: Open file explorer`.

**Approach Hints:**
- **Delete**: show confirmation dialog, call `std::filesystem::remove` / `remove_all`, refresh directory view.
- **Rename**: use an inline ImGui text input or modal; validate against name collisions and invalid chars.
- **Show in Explorer**: use `ShellExecuteW` on Windows, `xdg-open` on Linux for the parent directory.
- Refresh the asset grid and selection state after any mutation.
- Integrate with undo system if feasible, or at minimum log the action.

**Definition of Done:**
- [ ] Delete shows confirmation, removes file/folder, refreshes view.
- [ ] Rename allows inline editing, validates name, refreshes view.
- [ ] Show in Explorer opens OS file manager on Windows and Linux.
- [ ] Error cases (permission denied, name collision) show user-facing messages.

---

## [TRI-006] Add texture selector UI for SpriteRendererComponent in Inspector
Labels: type:feature, priority:P2-later, area:editor-tools, area:render-2d

**Context:**
The Inspector shows a placeholder "None (Select)" button for sprite textures. Without a real
selector, users cannot assign textures to sprites through the editor UI.

**Evidence:**
- `apps/editor/panels/InspectorPanel.cpp:331-337` — `// TODO: Texture selector` with placeholder button.

**Approach Hints:**
- Create a reusable asset-picker popup filtered by asset type (texture).
- Populate from `AssetManager` query for loaded/known texture assets.
- On selection, assign asset ID to `SpriteRendererComponent::textureId`.
- Add a "Clear" button to remove the texture assignment.
- Consider supporting drag-drop from the Asset Browser as an alternative path.

**Definition of Done:**
- [ ] Clicking the button opens a texture picker popup.
- [ ] Selecting a texture updates the sprite's texture reference and renders immediately.
- [ ] Clear/remove texture action works.
- [ ] Picker handles case where no textures are loaded gracefully.

---

## [TRI-007] Wire "Select Asset" prefab button to Asset Browser navigation
Labels: type:feature, priority:P2-later, area:editor-tools

**Context:**
The Inspector's prefab instance section has a "Select Asset" button that logs a message but
does not actually navigate the Asset Browser. Minor UX gap.

**Evidence:**
- `apps/editor/panels/InspectorPanel.cpp:772-776` — `// TODO: Select prefab in asset browser`.

**Approach Hints:**
- Add a method on AssetBrowserPanel to navigate to and highlight a specific asset by ID.
- Call it from the button handler, passing the `prefabId`.
- If the Asset Browser panel is collapsed/hidden, focus it first.
- Handle missing/invalid prefab IDs with a warning toast.

**Definition of Done:**
- [ ] Button navigates Asset Browser to the prefab's location and highlights it.
- [ ] Works when Asset Browser is in a different directory.
- [ ] Invalid prefab ID shows a user-facing warning.

---

## [TRI-008] Assign texture when creating sprite entity via viewport drag-drop
Labels: type:bug, priority:P1-important, area:editor-tools, area:render-2d

**Context:**
Dragging a texture asset into the viewport creates a new sprite entity but does not assign
the dropped texture to it. The entity appears blank. This is a correctness issue — the
feature exists but produces wrong results.

**Evidence:**
- `apps/editor/panels/ViewportPanel.cpp:205-209` — `// TODO: Load texture and assign to sprite`.

**Approach Hints:**
- After creating the entity with `SpriteRendererComponent`, look up the dropped asset path in `AssetManager`.
- If the texture is already loaded, assign its `AssetId` directly.
- If not loaded, trigger an async load and assign on completion (or load synchronously for editor use).
- Reject non-texture drops with a log warning or status bar message.

**Definition of Done:**
- [ ] Dropping a texture onto the viewport creates a sprite with that texture assigned.
- [ ] The texture is visible immediately (no blank entity).
- [ ] Non-texture assets are rejected with user feedback.

---

## [TRI-009] Move editor viewport rendering to framebuffer path
Labels: type:tech-debt, priority:P1-important, area:editor-tools, area:renderer

**Context:**
The editor viewport currently renders directly to the screen instead of to a framebuffer
texture displayed in the ImGui viewport panel. This blocks proper viewport resizing, multi-viewport
support, and post-processing in the editor.

**Evidence:**
- `apps/editor/panels/ViewportPanel.cpp:240-245` — `// TODO: Render to framebuffer and display as image`.

**Approach Hints:**
- Create a `Framebuffer` with color + depth attachments sized to the viewport panel dimensions.
- Bind framebuffer before `Renderer2D::beginScene`, unbind after `endScene`.
- Display the color attachment via `ImGui::Image()` in the viewport panel.
- Handle resize: recreate framebuffer when panel size changes (debounce to avoid thrashing).
- Ensure entity picking / mouse coordinate mapping accounts for framebuffer offset.

**Definition of Done:**
- [ ] Scene renders into a framebuffer; viewport panel displays the texture.
- [ ] Viewport resize works without artifacts or stale frames.
- [ ] Mouse picking coordinates remain correct after migration.
- [ ] No regression in editor FPS (framebuffer overhead < 0.5ms).

---

## [TRI-010] Complete "Apply All" in Prefab Overrides panel
Labels: type:bug, priority:P1-important, area:editor-tools, area:ecs-scene

**Context:**
The Prefab Overrides panel has an "Apply All" button that logs a message but does not actually
apply overrides back to the prefab asset. This breaks the prefab editing workflow — users expect
their instance changes to propagate.

**Evidence:**
- `apps/editor/panels/PrefabOverridesPanel.cpp:163-167` — `// TODO: Apply all overrides to the prefab asset`, logs "not yet fully implemented".

**Approach Hints:**
- Iterate collected overrides from the panel state.
- For each override, apply the value change to the source `Prefab` asset's component data.
- Mark the prefab asset as dirty so it gets saved.
- Refresh all instances of the prefab via `Prefab::updateInstances()`.
- Integrate with undo system — applying overrides should be undoable.

**Definition of Done:**
- [ ] "Apply All" writes override values back to the prefab asset data.
- [ ] All live instances of the prefab reflect the applied changes.
- [ ] The operation is undoable.
- [ ] Prefab asset is marked dirty for save.

---

## [TRI-011] Support AddComponent/RemoveComponent in prefab override pipeline
Labels: type:tech-debt, priority:P2-later, area:ecs-scene

**Context:**
The prefab override system only supports property-level overrides. Adding or removing components
on a prefab instance is silently ignored. This limits the usefulness of the prefab workflow
for anything beyond Transform/Sprite tweaks.

**Evidence:**
- `engine/src/scene/Prefab.cpp:665-669` — `// TODO: Handle AddComponent/RemoveComponent in v1.5`, logs warning.

**Approach Hints:**
- Extend the `OverrideKind` enum and serialization to include `AddComponent` and `RemoveComponent`.
- In `applyOverride()`, handle add by constructing the component and attaching to the entity.
- Handle remove by detaching the component from the entity.
- Ensure the serialization format is backward-compatible (old scenes without these kinds still load).
- Add round-trip tests: serialize override, deserialize, verify component presence/absence.

**Definition of Done:**
- [ ] AddComponent and RemoveComponent overrides are applied correctly.
- [ ] Override serialization/deserialization handles new kinds.
- [ ] Old scene files without these override kinds still load without error.
- [ ] Tests cover add, remove, and round-trip serialization.

---

## [TRI-012] Expand Prefab::updateInstances beyond Transform-only synchronization
Labels: type:bug, priority:P1-important, area:ecs-scene

**Context:**
`Prefab::updateInstances` only synchronizes the `Transform` component. Changes to any other
component on a prefab (SpriteRenderer, scripts, colliders, etc.) are not propagated to instances.
This makes the prefab system effectively non-functional for multi-component prefabs.

**Evidence:**
- `engine/src/scene/Prefab.cpp:580-604` — comment says "simplified update - just update Transform as example".

**Approach Hints:**
- Build a component-type registry or use EnTT's type-info to iterate all components on the prefab entity.
- For each component type, check the instance's override mask before writing.
- Handle nested prefab instances with a deterministic ordering (depth-first or breadth-first).
- Start with SpriteRendererComponent and at least one physics component; expand iteratively.
- Consider a visitor/reflection pattern to avoid per-type boilerplate.

**Definition of Done:**
- [ ] All registered component types are synchronized from prefab to instances.
- [ ] Instance overrides are respected (overridden properties not clobbered).
- [ ] Tests cover SpriteRenderer, at least one script, and one collider component.
- [ ] Nested prefab instances update in correct order.

---

## [TRI-013] Surface text input events from platform layer
Labels: type:feature, priority:P2-later, area:input, area:platform

**Context:**
The GLFW character callback receives UTF codepoints but intentionally discards them. No
text input is available to UI widgets or the scripting layer. This blocks any text-field
or chat-box gameplay feature.

**Evidence:**
- `engine/src/platform/Input.cpp:312-315` — callback ignores codepoints with comment "can be used for text fields later".

**Approach Hints:**
- Accumulate codepoints into a per-frame ring buffer or `std::vector<u32>` in the Input system.
- Expose `Input::getTextInput()` returning the buffered characters, cleared each frame.
- Wire ImGui's character input to this buffer (or let ImGui use GLFW directly).
- Document IME limitations (GLFW has basic IME support; note platform differences).

**Definition of Done:**
- [ ] Typed characters are captured and available via `Input::getTextInput()`.
- [ ] Buffer is cleared each frame.
- [ ] ImGui text fields continue to work (no double-input).
- [ ] Unit test verifies buffer accumulation and clearing.

---

## [TRI-014] Upgrade asset importers beyond raw file copy
Labels: type:tech-debt, priority:P2-later, area:asset-pipeline

**Context:**
All three asset importers (Texture, Shader, Audio) currently just copy source files to the
output directory without any processing. This means no mipmap generation, no shader validation,
no audio format normalization, and no metadata extraction. Consolidated from three identical
stub patterns.

**Evidence:**
- `engine/src/assets/AssetImporter.cpp:20-37` — TextureImporter copies file, comments list planned steps.
- `engine/src/assets/AssetImporter.cpp:75-80` — ShaderImporter copies file, comments list validation/precompile.
- `engine/src/assets/AssetImporter.cpp:113-118` — AudioImporter copies file, comments list conversion/metadata.

**Approach Hints:**
- **Texture**: Add import settings struct (generate mipmaps, max resolution, format). Use stb_image_resize or similar.
- **Shader**: Validate GLSL source at import time (call glslangValidator or parse with a lightweight checker). Cache compiled SPIR-V if cross-compilation is planned.
- **Audio**: Extract metadata (duration, sample rate, channels) and store in a sidecar `.meta` JSON. Normalize format if needed.
- Define an `ImportSettings` struct per asset type, serialized alongside the asset.
- Add pipeline tests that import a test asset and verify output contents/metadata.

**Definition of Done:**
- [ ] TextureImporter performs at least one processing step (e.g., metadata extraction or resize).
- [ ] ShaderImporter validates GLSL source and reports errors at import time.
- [ ] AudioImporter extracts and stores metadata (duration, sample rate, channels).
- [ ] Import settings are serializable and reproducible.
- [ ] Pipeline tests cover each importer.

---

## [TRI-015] Implement asset enumeration in ImGui debug Asset Browser panel
Labels: type:feature, priority:P2-later, area:editor-tools, area:asset-pipeline

**Context:**
The debug panels include an Asset Browser section that shows a placeholder message instead of
listing loaded assets. Useful for runtime debugging of asset state.

**Evidence:**
- `engine/src/imgui/DebugPanels.cpp:194-196` — `ImGui::TextDisabled("(Asset list not yet implemented)")`.

**Approach Hints:**
- Add an iteration API to `AssetManager` (e.g., `forEachAsset(callback)`).
- Display asset ID, type, path, and load state in an ImGui table.
- Add a text filter for search.
- Keep it lightweight — only query metadata, don't load assets for display.

**Definition of Done:**
- [ ] Debug Asset Browser lists all registered assets with ID, type, path, and state.
- [ ] Text filter narrows the list.
- [ ] Panel handles 500+ assets without visible lag.

---

## [TRI-016] Replace UI Label placeholder with real text rendering
Labels: type:tech-debt, priority:P2-later, area:ui, area:render-2d

**Context:**
The runtime UI `Label` widget draws a colored rectangle instead of actual text. This is
documented as a placeholder pending a font atlas / glyph rendering pipeline.

**Evidence:**
- `engine/include/limbo/ui/Widgets.hpp:26-30` — comment says "renders as colored rectangle for now".
- `engine/src/ui/Widgets.cpp:28-40` — draws a colored bar via `Renderer2D::drawQuad`.

**Approach Hints:**
- Integrate a font rasterizer (stb_truetype is already a dependency, or use FreeType).
- Build a font atlas texture at initialization; cache glyph metrics.
- In `Label::render()`, emit textured quads per glyph through `Renderer2D`.
- Support alignment (left/center/right), color, and basic clipping.
- Consider batching glyph quads into the existing sprite batch.

**Definition of Done:**
- [ ] `Label` renders actual text glyphs from a font atlas.
- [ ] Supports left/center/right alignment and color.
- [ ] Text is clipped to the widget bounds.
- [ ] At least one font is loaded and usable by default.

---

## [TRI-017] Improve thick-circle rendering quality in Renderer2D
Labels: type:perf, priority:P2-later, area:render-2d

**Context:**
Thick circles (rings) are approximated by drawing separate inner and outer line-segment loops.
This produces low visual quality at larger sizes and generates many draw calls.

**Evidence:**
- `engine/src/render/2d/Renderer2D.cpp:696-724` — comments describe the line-segment approximation.

**Approach Hints:**
- Replace with proper ring geometry: generate a triangle strip between inner and outer radii.
- Use adaptive segment count based on radius (more segments for larger circles).
- Batch the ring triangles into the existing quad batch (they share the same shader).
- Add a visual regression test scene with circles of various sizes and thicknesses.

**Definition of Done:**
- [ ] Thick circles render as smooth rings without visible line segments.
- [ ] Draw call count for a ring is 1 (batched).
- [ ] Visual quality verified at radii from 10px to 500px.
- [ ] No regression in batch renderer throughput.

---

## [TRI-018] Resolve 3D rendering skeleton APIs (Renderer3D, Mesh, Material, Model, Lighting, MeshRenderSystem)
Labels: type:tech-debt, priority:P2-later, area:render-3d, status:blocked

**Context:**
The engine exposes public 3D rendering headers (Renderer3D, Mesh, Material, Model, Lighting)
and an ECS MeshRenderSystem, but the `engine/src/render/3d/` directory is empty — no implementation
exists. These headers are explicitly marked EXPERIMENTAL. The risk is that users or internal code
accidentally depend on non-functional APIs.

**Evidence:**
- `engine/include/limbo/render/3d/Renderer3D.hpp:5-9` — "not yet implemented" marker.
- `engine/include/limbo/render/3d/Mesh.hpp:5-9` — placeholder marker.
- `engine/include/limbo/render/3d/Material.hpp:5-9` — placeholder marker.
- `engine/include/limbo/render/3d/Model.hpp:5-9` — placeholder marker.
- `engine/include/limbo/render/3d/Lighting.hpp:5-9` — placeholder marker.
- `engine/include/limbo/ecs/systems/3d/MeshRenderSystem.hpp:9-27` — class declared, no `.cpp`.
- `engine/src/render/3d/` — empty directory.

**Approach Hints:**
- **Option A (Hide):** Wrap all 3D headers in `#ifdef LIMBO_ENABLE_3D` and disable by default. Remove from default CMake target.
- **Option B (Implement MVP):** Start with Mesh + Material + basic forward renderer. Significant effort.
- Recommendation for a 2D-focused engine: Option A — hide until there is a concrete use case.
- If hiding, add a compile-time static_assert or `#error` in each header when the flag is off.
- Update docs/ROADMAP.md to reflect the decision.

**Definition of Done:**
- [ ] Decision documented (implement vs. hide).
- [ ] If hidden: headers gated behind compile flag, not included by default.
- [ ] If implemented: minimal render path works (draw a textured cube).
- [ ] No linker errors from accidental inclusion of 3D headers.
- [ ] docs/ROADMAP.md updated.

---

## [TRI-019] Resolve 3D physics skeleton APIs (Physics3D, PhysicsSystem3D)
Labels: type:tech-debt, priority:P2-later, area:physics, status:blocked

**Context:**
Similar to the 3D render APIs, the engine exposes Physics3D and PhysicsSystem3D headers
with no backing implementation. `engine/src/physics/3d/` is empty.

**Evidence:**
- `engine/include/limbo/physics/3d/Physics3D.hpp:17` — "skeleton for future implementation".
- `engine/include/limbo/physics/3d/PhysicsSystem3D.hpp:14` — "skeleton for future implementation".
- `engine/src/physics/3d/` — empty directory.

**Approach Hints:**
- Apply the same gating strategy as TRI-018 (shared `LIMBO_ENABLE_3D` flag or separate `LIMBO_ENABLE_PHYSICS3D`).
- If implementing: integrate Jolt Physics or Bullet as backend; start with rigid body stepping + gravity.
- Recommendation: gate behind flag, same as 3D render.

**Definition of Done:**
- [ ] Decision documented (implement vs. hide), aligned with TRI-018.
- [ ] If hidden: headers gated behind compile flag.
- [ ] If implemented: rigid body stepping + gravity test passes.
- [ ] No linker errors from accidental inclusion.

---

## [TRI-020] Expand clang-tidy CI scope beyond 50-file limit
Labels: type:tech-debt, priority:P2-later, area:build-ci

**Context:**
The CI workflow runs clang-tidy on only the first 50 `.cpp` files found by `find`, leaving
the majority of the codebase unchecked. New code added outside this arbitrary window gets
no static analysis.

**Evidence:**
- `.github/workflows/ci.yml:101-102` — `find engine/src apps -name "*.cpp" | head -50`.

**Approach Hints:**
- Remove the `head -50` truncation entirely, or replace with deterministic sharding (e.g., split file list into N chunks, run in matrix).
- Use `compile_commands.json` with `run-clang-tidy` for parallel execution.
- Cache clang-tidy results between runs (only re-analyze changed files via `git diff --name-only`).
- Log total files analyzed in CI output for visibility.

**Definition of Done:**
- [ ] clang-tidy runs on all `.cpp` files in the codebase (or a documented deterministic subset).
- [ ] CI runtime remains under 15 minutes.
- [ ] File count is logged in CI output.
- [ ] No new clang-tidy warnings introduced.

---
