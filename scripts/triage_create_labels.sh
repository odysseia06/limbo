#!/usr/bin/env bash
set -euo pipefail

if ! command -v gh >/dev/null 2>&1; then
  echo "[manual] GitHub CLI (gh) not found."
  echo "Create labels manually using docs/triage/TRIAGE_REPORT.md taxonomy."
  exit 0
fi

if ! gh auth status >/dev/null 2>&1; then
  echo "[manual] gh is installed but not authenticated."
  echo "Run: gh auth login"
  echo "Then rerun: ./scripts/triage_create_labels.sh"
  exit 0
fi

LABEL_SPECS=$(cat <<'SPECS'
type:bug|d73a4a|Defect or regression
type:feature|a2eeef|New capability or behavior
type:perf|f9d0c4|Performance issue or optimization
type:tech-debt|cfd3d7|Maintainability or architecture debt
type:docs|0075ca|Documentation work
priority:P0-crash-blocker|b60205|Crash or release blocker
priority:P1-important|d93f0b|Important near-term work
priority:P2-later|fbca04|Valuable but can wait
area:renderer|5319e7|Rendering pipeline and graphics APIs
area:ecs-scene|1d76db|ECS, hierarchy, scene, prefab
area:physics|0e8a16|Physics systems and components
area:editor-tools|c2e0c6|Editor and internal tooling UX
area:asset-pipeline|fef2c0|Importing, registry, hot reload, cooking
area:platform|0b7285|Platform integration and OS-specific behavior
area:build-ci|0052cc|Build scripts and CI workflows
area:core-runtime|0366d6|Core runtime, threading, app lifecycle
area:ui|e99695|UI systems and widgets
area:input|bfdadc|Input devices and mapping
area:audio|f9b3d1|Audio engine and playback
area:scripting|d4c5f9|Lua scripting runtime and bindings
area:math|b5f2ea|Math primitives and transformations
status:blocked|000000|Cannot proceed due to dependency or external blocker
status:needs-investigation|fbca04|Needs deeper root-cause analysis
platform:windows|00aaff|Windows-specific impact
platform:linux|2ea44f|Linux-specific impact
SPECS
)

existing_file=$(mktemp)
trap 'rm -f "$existing_file"' EXIT

gh label list --limit 500 --json name --jq '.[].name' > "$existing_file"

created=0
skipped=0

while IFS='|' read -r name color desc; do
  [[ -z "$name" ]] && continue

  if grep -Fxq "$name" "$existing_file"; then
    echo "[skip] $name"
    skipped=$((skipped + 1))
    continue
  fi

  gh label create "$name" --color "$color" --description "$desc" >/dev/null
  echo "[create] $name"
  echo "$name" >> "$existing_file"
  created=$((created + 1))
done <<< "$LABEL_SPECS"

echo

echo "Labels created: $created"
echo "Labels skipped: $skipped"
