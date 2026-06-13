#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIMIT="${LIMIT:-1000}"
TOP="${TOP:-40}"

usage() {
  cat <<'EOF'
Usage:
  ./linux/audit_large_source_files.sh [options]

Options:
  --limit N  show files with at least N lines (default: 1000)
  --top N    show at most N files (default: 40)
  --all      include docs, raw results, and archived files
  --help     show this message
EOF
}

INCLUDE_ALL=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --limit)
      [[ $# -ge 2 ]] || { echo "missing value for --limit" >&2; exit 1; }
      LIMIT="$2"
      shift 2
      ;;
    --top)
      [[ $# -ge 2 ]] || { echo "missing value for --top" >&2; exit 1; }
      TOP="$2"
      shift 2
      ;;
    --all)
      INCLUDE_ALL=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

case "$LIMIT" in
  ''|*[!0-9]*) echo "--limit must be a positive integer" >&2; exit 1 ;;
esac
case "$TOP" in
  ''|*[!0-9]*) echo "--top must be a positive integer" >&2; exit 1 ;;
esac

find_args=(
  "$ROOT_DIR"
  -path "$ROOT_DIR/.git" -prune -o
  -path '*/out/*' -prune -o
  -path '*/out_*/*' -prune -o
)

if [[ "$INCLUDE_ALL" -eq 0 ]]; then
  find_args+=(
    -path '*/docs/archive/*' -prune -o
    -path '*/private/raw-results/*' -prune -o
    -path '*/results/*' -prune -o
  )
fi

find_args+=(
  -type f
  \( -name '*.c' -o -name '*.h' -o -name '*.inc' -o -name '*.sh' -o -name '*.ps1' -o -name 'Makefile' \)
  -print0
)

find "${find_args[@]}" \
  | xargs -0 -r wc -l \
  | awk -v limit="$LIMIT" '$2 != "total" && $1 >= limit { print $1, $2 }' \
  | sed "s# ${ROOT_DIR}/# #" \
  | sort -nr \
  | head -n "$TOP"
