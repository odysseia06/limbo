# Limbo Repo Triage Report

## Scope
This report sets up a practical GitHub issue triage system for the repository without changing engine source code.
Artifacts included in this pass:
- Label taxonomy and creation automation.
- Issue templates for bug, feature, perf, and tech-debt.
- A curated, evidence-backed backlog in `docs/triage/BACKLOG.md`.
- Idempotent scripts to create labels and issues with GitHub CLI.

## Repo Inventory
### Build and Tooling
- Build system: CMake (`CMakeLists.txt:1-57`, `engine/CMakeLists.txt:1-323`).
- Languages: C and C++20 (`CMakeLists.txt:7`, `CMakeLists.txt:10-13`).
- CI: GitHub Actions (`.github/workflows/ci.yml:1-172`).

### Main Folders
- Engine: `engine/include/limbo/*`, `engine/src/*`.
- Apps: `apps/editor`, `apps/sandbox`.
- Tests: `tests/*`.
- Tools: `tools/assetcooker`.
- Docs: `docs/*`.

### Engine Modules Detected
From `engine/CMakeLists.txt:10-123` and include tree:
- renderer (`render/common`, `render/2d`, `render/3d` skeleton)
- ecs/scene (`ecs`, `scene`)
- physics (`physics/2d`, `physics/3d` skeleton)
- input (`input`, `platform/Input`)
- editor/tools (`apps/editor`, `tools/assetcooker`)
- asset pipeline (`assets`, `FileWatcher`, `HotReloadManager`, importer)
- platform (`platform`)
- build/ci (`cmake`, `.github/workflows`)
- math (`tests/math`, glm usage)
- audio (`audio`)
- scripting (`scripting`)
- ui/imgui (`ui`, `imgui`)

## Candidate Extraction Method
Signals used:
- Explicit TODO markers (`TODO`, `FIXME`, etc.).
- Placeholder/skeleton comments and APIs.
- Missing implementation paths visible in build lists.
- CI workflow constraints that reduce coverage.
- Clear runtime/perf risks (main-thread fallbacks, arbitrary ordering paths).

Primary evidence examples:
- Editor TODOs: `apps/editor/EditorApp.cpp`, `apps/editor/panels/*`.
- Scene/3D placeholders: `engine/include/limbo/scene/Scene.hpp`, `engine/include/limbo/render/3d/*`.
- 3D/physics skeleton exposure in umbrella header: `engine/include/limbo/Limbo.hpp`.
- CI limitations: `.github/workflows/ci.yml:92-93`, `.github/workflows/ci.yml:124-126`.

## Label Taxonomy
The taxonomy is intentionally minimal and composable.

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
- `area:ecs-scene`
- `area:physics`
- `area:editor-tools`
- `area:asset-pipeline`
- `area:platform`
- `area:build-ci`
- `area:core-runtime`
- `area:ui`
- `area:input`
- `area:audio`
- `area:scripting`
- `area:math`

### Status
- `status:blocked`
- `status:needs-investigation`

### Platform (only where relevant)
- `platform:windows`
- `platform:linux`

Total labels: **25**.

## Issue Templates Added
Added in `.github/ISSUE_TEMPLATE/`:
- `bug_report.yml`
- `feature_request.yml`
- `perf.yml`
- `tech_debt.yml`

Template defaults:
- Bug: `type:bug`, `status:needs-investigation`
- Feature: `type:feature`
- Perf: `type:perf`, `status:needs-investigation`
- Tech debt: `type:tech-debt`

## Backlog
Curated initial backlog is in `docs/triage/BACKLOG.md`.
- Backlog items: **28**
- All items include evidence + Definition of Done.

## Suggested Milestones
- `M1 Editor Workflow Completion`
  - Close editor TODOs affecting day-to-day authoring.
- `M2 3D and Scene Foundations`
  - Resolve skeleton API exposure and establish real implementation boundaries.
- `M3 Reliability and CI Hardening`
  - Improve CI signal quality and remove known perf/reload hazards.

## Suggested Project Board Columns
- `Triage`
- `Ready`
- `In Progress`
- `Blocked`
- `Review`
- `Done`

## Automation Scripts
- `scripts/triage_create_labels.sh`
  - Creates missing labels only.
- `scripts/triage_create_issues.sh`
  - Parses `docs/triage/BACKLOG.md` issues.
  - Skips existing issues by exact title match.

Both scripts:
- Detect `gh` availability.
- Detect authentication with `gh auth status`.
- Fall back to manual instructions when unavailable.

