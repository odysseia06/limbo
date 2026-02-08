# Limbo Repository Triage Report

Generated: 2026-02-08. Reviewed and consolidated: 2026-02-08.

## 1) Repository Inventory

### Build and Tooling
- Build system: **CMake** (`CMakeLists.txt`, `CMakePresets.json`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`)
- CI: GitHub Actions (`.github/workflows/ci.yml`)
- Test framework: Catch2 via CMake (`tests/CMakeLists.txt:43-46`)

### Languages and Asset Formats
- C++ (`.cpp`, `.hpp`) dominant
- Lua scripts (`assets/demo/platformer/scripts/*.lua`, `apps/sandbox/assets/scripts/*.lua`)
- GLSL shaders (`assets/shaders/**/*.vert|frag`)
- JSON scene/assets (`assets/scenes/*.json`)

### Main Folders
- `engine/` engine runtime modules
- `apps/editor/` in-engine editor
- `apps/sandbox/` sample app
- `tools/assetcooker/` asset tooling
- `tests/` Catch2 tests
- `assets/` sample/runtime assets
- `.github/workflows/` CI

### Engine Module Map (from `engine/CMakeLists.txt:10-123`)
- Core, Util, Platform, Input, Runtime
- Render common, Render 2D, Render 3D (headers only skeleton)
- Graphics/Framebuffer
- ECS + Scene
- Physics 2D + Physics 3D (headers only skeleton)
- Audio, Animation, Scripting, Particles, Tilemap, UI, ImGui debug panels

## 2) Candidate Extraction Summary

### Evidence Collection Method
- Marker scan for `TODO|FIXME|HACK|DEBT|OPTIMIZE`
- Stub/skeleton scan for "not yet implemented", "placeholder", and empty implementation directories
- Manual skim of key subsystems: renderer, scene/prefab, asset pipeline, input/platform, editor, CI

### Notable Findings
- Editor has multiple functional TODOs in active workflows (`apps/editor/**/*.cpp`)
- Prefab override pipeline is partial (`engine/src/scene/Prefab.cpp`)
- Asset importers currently copy files without preprocessing (`engine/src/assets/AssetImporter.cpp`)
- Render/physics 3D APIs are exposed as skeleton headers while source dirs are empty
- CI clang-tidy scope is intentionally truncated to first 50 files (`.github/workflows/ci.yml:101-102`)

### Test/Execution Note
- In this local environment, `ctest` command was unavailable (`ctest: command not found`), so no local failing-test evidence was added.

## 3) Label Taxonomy

### Type (exactly one per issue)
| Label | Description |
|-------|-------------|
| `type:bug` | Functional defect or regression |
| `type:feature` | New capability or behavior |
| `type:perf` | Performance optimization or regression |
| `type:tech-debt` | Refactor, cleanup, or structural debt |
| `type:docs` | Documentation work |

### Priority (exactly one per issue)
| Label | Criteria |
|-------|----------|
| `priority:P0-crash-blocker` | Crash, build break, or data loss |
| `priority:P1-important` | Correctness issue, perf cliff, or platform blocker |
| `priority:P2-later` | Refactor, nice-to-have, or deferrable improvement |

### Area (1-3 per issue)
| Label | Scope |
|-------|-------|
| `area:renderer` | Rendering pipeline and graphics runtime |
| `area:render-2d` | 2D rendering systems |
| `area:render-3d` | 3D rendering systems |
| `area:ecs-scene` | ECS, scene graph, prefab, serialization |
| `area:physics` | Physics systems and components |
| `area:editor-tools` | Editor UI and tooling workflows |
| `area:asset-pipeline` | Importing, registry, hot-reload, cooking |
| `area:platform` | Cross-platform runtime abstraction |
| `area:platform-windows` | Windows-specific behavior |
| `area:platform-linux` | Linux-specific behavior |
| `area:build-ci` | Build scripts, CI, automation |
| `area:math` | Math primitives/transforms |
| `area:audio` | Audio runtime and import |
| `area:input` | Input handling and bindings |
| `area:ui` | Runtime UI/widget systems |
| `area:scripting` | Lua/scripting runtime and tooling |
| `area:testing` | Tests, harness, and coverage |

### Status (only when justified)
| Label | Use |
|-------|-----|
| `status:blocked` | Blocked by external dependency or design decision |
| `status:needs-investigation` | Needs validation or deeper diagnosis |

## 4) Issue Templates

Created:
- `.github/ISSUE_TEMPLATE/bug_report.yml`
- `.github/ISSUE_TEMPLATE/feature_request.yml`
- `.github/ISSUE_TEMPLATE/perf.yml`
- `.github/ISSUE_TEMPLATE/tech_debt.yml`

Each template requires: evidence (paths/lines, logs, repro), scoped impact, and explicit Definition of Done.

## 5) Backlog Summary

**Backlog file:** `docs/triage/BACKLOG.md`
**Total issues:** 20

### Consolidation Changes (from original 27 items)

| Change | Original | Result | Rationale |
|--------|----------|--------|-----------|
| Merged | TRI-005 (delete), TRI-006 (rename), TRI-007 (show in explorer) | TRI-005 (combined) | Three stubs in the same context menu, same file, adjacent lines |
| Merged | TRI-019 (texture import), TRI-020 (shader import), TRI-021 (audio import) | TRI-014 (combined) | Identical pattern (raw copy) across three importers |
| Merged | TRI-022 (Renderer3D), TRI-023 (Mesh/Material/Model/Lighting), TRI-024 (MeshRenderSystem) | TRI-018 (combined) | All 3D render skeleton APIs — same decision gate, same empty src dir |
| Merged | TRI-025 (Physics3D), TRI-026 (PhysicsSystem3D) | TRI-019 (combined) | Both 3D physics skeleton APIs — same pattern as 3D render |

### Priority Adjustments

| Issue | Old Priority | New Priority | Rationale |
|-------|-------------|-------------|-----------|
| TRI-005 (asset browser actions) | P1-important | P2-later | Feature stubs, not broken functionality |
| TRI-008 (texture selector) | P1-important | P2-later | Missing feature, not a correctness bug |
| TRI-010 (drag-drop texture) | P1-important | P1-important | Kept — feature exists but produces wrong result |
| TRI-013 (add/remove overrides) | P1-important | P2-later | Partial feature, property overrides work |
| TRI-018 (text input events) | P1-important | P2-later | No text input UI consumers exist yet |
| TRI-027 (clang-tidy scope) | P1-important | P2-later | CI still runs, just limited coverage |

### Type Reclassifications

| Issue | Old Type | New Type | Rationale |
|-------|----------|----------|-----------|
| TRI-005 (asset browser actions) | type:bug | type:feature | Stubs for unimplemented features, not regressions |
| TRI-003 (Save As) | type:bug | type:feature | Feature was never implemented, not broken |

### Issue Distribution

**By Priority:**
- P0: 0 (no crashes, build breaks, or data loss identified)
- P1: 6 (TRI-002, TRI-003, TRI-008, TRI-009, TRI-010, TRI-012)
- P2: 14

**By Type:**
- type:bug: 4 (TRI-002, TRI-008, TRI-010, TRI-012)
- type:feature: 8 (TRI-001, TRI-003, TRI-004, TRI-005, TRI-006, TRI-007, TRI-013, TRI-015)
- type:tech-debt: 7 (TRI-009, TRI-011, TRI-014, TRI-016, TRI-018, TRI-019, TRI-020)
- type:perf: 1 (TRI-017)

**Label Consistency Check:**
- Every issue has exactly 1 type label: PASS
- Every issue has exactly 1 priority label: PASS
- Every issue has 1-3 area labels: PASS
- status labels used only where justified (TRI-018, TRI-019 — blocked on design decision): PASS

## 6) Suggested Milestones

### M1: Editor Workflow Reliability
Issues: TRI-002, TRI-003, TRI-008, TRI-009, TRI-010, TRI-012
Focus: Prevent data loss, complete prefab workflow, fix viewport rendering.

### M2: Editor Polish
Issues: TRI-001, TRI-004, TRI-005, TRI-006, TRI-007, TRI-015
Focus: Quality-of-life improvements for day-to-day editor use.

### M3: Asset Pipeline Hardening
Issues: TRI-014
Focus: Move importers beyond raw copy to real processing.

### M4: Runtime Completeness
Issues: TRI-011, TRI-013, TRI-016, TRI-017
Focus: Fill gaps in runtime features (text rendering, prefab overrides, input, circles).

### M5: 3D/Physics Scope Decision
Issues: TRI-018, TRI-019
Focus: Decide whether to implement or gate experimental 3D APIs.

### M6: CI and Quality Coverage
Issues: TRI-020
Focus: Expand static analysis scope.

### Board Columns
1. `triage/inbox`
2. `triage/ready`
3. `in-progress`
4. `blocked`
5. `review`
6. `done`

## 7) Automation Scripts

### `scripts/triage_create_labels.sh`
- Idempotent: skips existing labels via `gh label list` lookup.
- Graceful degradation: exits cleanly if `gh` is missing or unauthenticated.
- Error handling: individual label creation failures are logged and counted; script continues processing remaining labels. Exits non-zero if any failures occurred.

### `scripts/triage_create_issues.sh`
- Idempotent: skips issues whose title already exists (checks all states via `--state all --limit 5000`).
- Graceful degradation: exits cleanly if `gh` is missing or unauthenticated.
- Error handling: individual issue creation failures are logged and counted; script continues processing remaining issues. Exits non-zero if any failures occurred.
- Parsing: reads `docs/triage/BACKLOG.md`, matches `## [TRI-NNN] Title` headings and `Labels:` lines.
