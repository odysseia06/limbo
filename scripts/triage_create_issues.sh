#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKLOG_FILE="$ROOT_DIR/docs/triage/BACKLOG.md"
cd "$ROOT_DIR"

if ! command -v gh >/dev/null 2>&1; then
    echo "[triage] gh CLI not found."
    echo "[triage] Manual step: create issues from docs/triage/BACKLOG.md."
    exit 0
fi

if ! gh auth status >/dev/null 2>&1; then
    echo "[triage] gh CLI is installed but not authenticated."
    echo "[triage] Run: gh auth login"
    exit 0
fi

if [[ ! -f "$BACKLOG_FILE" ]]; then
    echo "[triage] Missing backlog file: $BACKLOG_FILE" >&2
    exit 1
fi

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

existing_titles_file="$tmp_dir/existing_titles.txt"
gh issue list --state all --limit 5000 --json title --jq '.[].title' > "$existing_titles_file"

issue_count=0
created_count=0
skipped_count=0
failed_count=0

current_id=""
current_title=""
current_labels=""
current_body=""

trim() {
    local s="$1"
    s="${s#"${s%%[![:space:]]*}"}"
    s="${s%"${s##*[![:space:]]}"}"
    printf '%s' "$s"
}

flush_issue() {
    if [[ -z "$current_title" ]]; then
        return
    fi

    issue_count=$((issue_count + 1))

    if [[ -z "$current_labels" ]]; then
        echo "[triage] skip $current_id: missing Labels line"
        skipped_count=$((skipped_count + 1))
        current_id=""
        current_title=""
        current_labels=""
        current_body=""
        return
    fi

    if grep -Fxq "$current_title" "$existing_titles_file"; then
        echo "[triage] skip issue (exists): $current_title"
        skipped_count=$((skipped_count + 1))
        current_id=""
        current_title=""
        current_labels=""
        current_body=""
        return
    fi

    body_file="$tmp_dir/${current_id}.md"
    {
        echo "Seed: $current_id"
        echo
        echo "$current_body"
    } > "$body_file"

    cmd=(gh issue create --title "$current_title" --body-file "$body_file")
    IFS=',' read -r -a label_list <<< "$current_labels"
    for raw_label in "${label_list[@]}"; do
        label="$(trim "$raw_label")"
        if [[ -n "$label" ]]; then
            cmd+=(--label "$label")
        fi
    done

    if "${cmd[@]}" >/dev/null 2>&1; then
        echo "$current_title" >> "$existing_titles_file"
        echo "[triage] created issue: $current_title"
        created_count=$((created_count + 1))
    else
        echo "[triage] FAILED to create issue: $current_title" >&2
        failed_count=$((failed_count + 1))
    fi

    current_id=""
    current_title=""
    current_labels=""
    current_body=""
}

while IFS= read -r line || [[ -n "$line" ]]; do
    if [[ "$line" =~ ^##[[:space:]]+\[(TRI-[0-9]{3})\][[:space:]]+(.+)$ ]]; then
        flush_issue
        current_id="${BASH_REMATCH[1]}"
        current_title="${BASH_REMATCH[2]}"
        current_labels=""
        current_body=""
        continue
    fi

    if [[ -z "$current_title" ]]; then
        continue
    fi

    if [[ "$line" =~ ^Labels:[[:space:]]*(.+)$ ]]; then
        current_labels="${BASH_REMATCH[1]}"
        continue
    fi

    if [[ "$line" == "---" ]]; then
        continue
    fi

    current_body+="$line"$'\n'
done < "$BACKLOG_FILE"

flush_issue

echo "[triage] issue creation complete"
echo "[triage] issue seeds parsed: $issue_count"
echo "[triage] issues created: $created_count"
echo "[triage] issues skipped: $skipped_count"
if [[ "$failed_count" -gt 0 ]]; then
    echo "[triage] issues FAILED: $failed_count" >&2
    exit 1
fi
