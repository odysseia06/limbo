# Limbo Repository Triage Report

Generated on 2026-02-08.

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

## 3) Label Taxonomy (Minimal + Practical)

### Type
- `type:bug`
- `type:feature`
- `type:perf`
- `type:tech-debt`
- `type:docs`

### Priority
- `priority:P0-crash-blocker`
- `priority:P1-important`
- `priority:P2-later`

### Area
- `area:renderer`
- `area:render-2d`
- `area:render-3d`
- `area:ecs-scene`
- `area:physics`
- `area:editor-tools`
- `area:asset-pipeline`
- `area:platform`
- `area:platform-windows`
- `area:platform-linux`
- `area:build-ci`
- `area:math`
- `area:audio`
- `area:input`
- `area:ui`
- `area:scripting`
- `area:testing`

### Status
- `status:blocked`
- `status:needs-investigation`

## 4) Issue Templates Added

Created:
- `.github/ISSUE_TEMPLATE/bug_report.yml`
- `.github/ISSUE_TEMPLATE/feature_request.yml`
- `.github/ISSUE_TEMPLATE/perf.yml`
- `.github/ISSUE_TEMPLATE/tech_debt.yml`

Each template asks for evidence (paths/lines, logs, repro), scoped impact, and explicit Definition of Done.

## 5) Initial Backlog

- Backlog file: `docs/triage/BACKLOG.md`
- Curated issue seeds: **27**
- Each item includes:
  - labels
  - direct evidence (`file:line-range` + TODO/stub/symptom)
  - Definition of Done checklist

## 6) Suggested Milestones and Board Flow

### Milestones
1. `M1 Editor Workflow Reliability`
   - Unsaved-change handling, Save As flow, asset browser core actions, viewport rendering path
2. `M2 Asset Pipeline Hardening`
   - Texture/shader/audio import processing beyond raw copy
3. `M3 3D/Physics Scope Decision`
   - Decide implement vs hide experimental 3D/physics APIs
4. `M4 CI and Quality Coverage`
   - Expand static analysis scope and targeted tests

### Board Columns
1. `triage/inbox`
2. `triage/ready`
3. `in-progress`
4. `blocked`
5. `review`
6. `done`

## 7) Automation Scripts

Created:
- `scripts/triage_create_labels.sh`
- `scripts/triage_create_issues.sh`

Behavior:
- Both scripts are idempotent.
- Both detect `gh` availability + auth.
- Labels: create only missing labels.
- Issues: parse `docs/triage/BACKLOG.md` and skip existing issue titles.
- If `gh` is unavailable/unauthed, scripts print manual next steps and exit cleanly.
