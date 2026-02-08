#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if ! command -v gh >/dev/null 2>&1; then
    echo "[triage] gh CLI not found."
    echo "[triage] Manual step: create labels listed in docs/triage/TRIAGE_REPORT.md."
    exit 0
fi

if ! gh auth status >/dev/null 2>&1; then
    echo "[triage] gh CLI is installed but not authenticated."
    echo "[triage] Run: gh auth login"
    exit 0
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

existing_labels_file="$tmp_dir/existing_labels.txt"
gh label list --limit 1000 --json name --jq '.[].name' > "$existing_labels_file"

labels=(
    "type:bug|d73a4a|Functional defect or regression."
    "type:feature|1d76db|New capability or behavior."
    "type:perf|fbca04|Performance optimization or regression."
    "type:tech-debt|6c757d|Refactor, cleanup, or structural debt."
    "type:docs|0e8a16|Documentation work."
    "priority:P0-crash-blocker|b60205|Release blocker: crash/data loss/build break."
    "priority:P1-important|d93f0b|Important, should land soon."
    "priority:P2-later|fbca04|Useful but deferrable."
    "area:renderer|0052cc|Rendering pipeline and graphics runtime."
    "area:render-2d|1f6feb|2D rendering systems."
    "area:render-3d|5319e7|3D rendering systems."
    "area:ecs-scene|0b7285|ECS, scene graph, prefab, serialization."
    "area:physics|c2e0c6|Physics systems and components."
    "area:editor-tools|2b8a3e|Editor UI and tooling workflows."
    "area:asset-pipeline|a371f7|Importing, registry, hot-reload, cooking."
    "area:platform|d4c5f9|Cross-platform runtime abstraction."
    "area:platform-windows|f9d0c4|Windows-specific behavior."
    "area:platform-linux|c5def5|Linux-specific behavior."
    "area:build-ci|7057ff|Build scripts, CI, automation."
    "area:math|bfdadc|Math primitives/transforms."
    "area:audio|fef2c0|Audio runtime and import."
    "area:input|a2eeef|Input handling and bindings."
    "area:ui|f9d0c4|Runtime UI/widget systems."
    "area:scripting|bfd4f2|Lua/scripting runtime and tooling."
    "area:testing|ededed|Tests, harness, and coverage."
    "status:blocked|000000|Blocked by dependency/decision."
    "status:needs-investigation|e4e669|Needs validation or deeper diagnosis."
)

created_count=0
skipped_count=0
failed_count=0

for spec in "${labels[@]}"; do
    IFS='|' read -r name color description <<< "$spec"

    if grep -Fxq "$name" "$existing_labels_file"; then
        echo "[triage] skip label: $name"
        skipped_count=$((skipped_count + 1))
        continue
    fi

    if gh label create "$name" --color "$color" --description "$description" >/dev/null 2>&1; then
        echo "$name" >> "$existing_labels_file"
        echo "[triage] created label: $name"
        created_count=$((created_count + 1))
    else
        echo "[triage] FAILED to create label: $name" >&2
        failed_count=$((failed_count + 1))
    fi
done

echo "[triage] label creation complete"
echo "[triage] total labels configured: ${#labels[@]}"
echo "[triage] labels created: $created_count"
echo "[triage] labels skipped: $skipped_count"
if [[ "$failed_count" -gt 0 ]]; then
    echo "[triage] labels FAILED: $failed_count" >&2
    exit 1
fi
