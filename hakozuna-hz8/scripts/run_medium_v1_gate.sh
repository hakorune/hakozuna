#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUT="$ROOT/bench_results/${STAMP}_medium_v1_gate"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"

mkdir -p "$OUT"

make -C "$ROOT" bench-release

run_row() {
  local name="$1"
  shift
  "$ROOT/h8_bench_release" \
    --runs "$RUNS" \
    --threads "$THREADS" \
    --iters "$ITERS" \
    "$@" > "$OUT/${name}.txt"
}

run_row medium_local0 \
  --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 1

run_row medium_interleaved_remote50 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

run_row medium_phase_remote90 \
  --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0

run_row main_interleaved_remote90 \
  --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 \
  --live-window 4096

{
  echo "# MediumRun V1 Gate"
  echo
  echo '```text'
  echo "runs: $RUNS"
  echo "threads: $THREADS"
  echo "iters: $ITERS"
  echo "bench: h8_bench_release"
  echo '```'
  echo
  for file in "$OUT"/*.txt; do
    echo "## $(basename "$file" .txt)"
    echo
    echo '```text'
    grep -E 'summary|medium_geometry_id|throughput median|steady_work|peak_rss median|post_rss median|page_faults|phase_ms|interleaved_phase_ms' "$file" || true
    echo '```'
    echo
  done
} > "$OUT/README.md"

echo "$OUT"
