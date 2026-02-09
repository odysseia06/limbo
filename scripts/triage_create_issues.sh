#!/usr/bin/env bash
set -euo pipefail

BACKLOG_FILE="docs/triage/BACKLOG.md"

if [[ ! -f "$BACKLOG_FILE" ]]; then
  echo "Backlog file not found: $BACKLOG_FILE"
  exit 1
fi

if ! command -v gh >/dev/null 2>&1; then
  echo "[manual] GitHub CLI (gh) not found."
  echo "Create issues manually from headings in $BACKLOG_FILE."
  exit 0
fi

if ! gh auth status >/dev/null 2>&1; then
  echo "[manual] gh is installed but not authenticated."
  echo "Run: gh auth login"
  echo "Then rerun: ./scripts/triage_create_issues.sh"
  exit 0
fi

tmpdir=$(mktemp -d)
existing_titles=$(mktemp)
trap 'rm -rf "$tmpdir"; rm -f "$existing_titles"' EXIT

awk -v outdir="$tmpdir" '
function flush() {
  if (issue_id == "") return;
  meta = outdir "/" issue_id ".meta";
  bodyf = outdir "/" issue_id ".md";
  print "title=" title > meta;
  print "labels=" labels >> meta;
  close(meta);
  print body > bodyf;
  close(bodyf);
}
$0 ~ /^### ISSUE-[0-9]+: / {
  flush();
  header = $0;
  sub(/^### /, "", header);

  split_pos = index(header, ": ");
  issue_id = header;
  title = "";
  if (split_pos > 0) {
    issue_id = substr(header, 1, split_pos - 1);
    title = substr(header, split_pos + 2);
  }

  labels = "";
  body = $0 "\n";
  in_issue = 1;
  next;
}
{
  if (in_issue) {
    body = body $0 "\n";
    if ($0 ~ /^Labels:[[:space:]]*/) {
      labels = $0;
      sub(/^Labels:[[:space:]]*/, "", labels);
    }
  }
}
END {
  flush();
}
' "$BACKLOG_FILE"

gh issue list --state all --limit 1000 --json title --jq '.[].title' > "$existing_titles"

created=0
skipped=0
invalid=0

while IFS= read -r meta; do
  id=$(basename "$meta" .meta)
  body_file="$tmpdir/$id.md"

  title=$(sed -n 's/^title=//p' "$meta" | head -n1)
  labels_csv=$(sed -n 's/^labels=//p' "$meta" | head -n1)

  if [[ -z "$title" || -z "$labels_csv" ]]; then
    echo "[warn] $id is missing title or labels; skipping"
    invalid=$((invalid + 1))
    continue
  fi

  if grep -Fxq "$title" "$existing_titles"; then
    echo "[skip] $title"
    skipped=$((skipped + 1))
    continue
  fi

  label_args=()
  IFS=',' read -r -a label_list <<< "$labels_csv"
  for label in "${label_list[@]}"; do
    label="${label#${label%%[![:space:]]*}}"
    label="${label%${label##*[![:space:]]}}"
    [[ -z "$label" ]] && continue
    label_args+=(--label "$label")
  done

  if [[ ${#label_args[@]} -eq 0 ]]; then
    echo "[warn] $id has no valid labels; skipping"
    invalid=$((invalid + 1))
    continue
  fi

  gh issue create --title "$title" "${label_args[@]}" --body-file "$body_file" >/dev/null
  echo "$title" >> "$existing_titles"
  echo "[create] $title"
  created=$((created + 1))
done < <(find "$tmpdir" -maxdepth 1 -type f -name 'ISSUE-*.meta' | sort)

echo

echo "Issues created: $created"
echo "Issues skipped (already existed): $skipped"
echo "Issues skipped (invalid blocks): $invalid"
