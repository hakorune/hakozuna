#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUT="$ROOT/bench_results/${STAMP}_medium_chunk_paired_gate"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"

mkdir -p "$OUT"

make -C "$ROOT" bench-release bench-release-mediumchunk

run_pair() {
  local name="$1"
  shift
  "$ROOT/h8_bench_release" \
    --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" "$@" \
    > "$OUT/${name}_baseline.txt"
  "$ROOT/h8_bench_release_mediumchunk" \
    --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" "$@" \
    > "$OUT/${name}_chunk.txt"
}

run_pair medium_interleaved_remote50 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

run_pair main_interleaved_remote90 \
  --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 \
  --live-window 4096

run_pair small_guard_local0 \
  --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 1

run_pair small_interleaved_remote90 \
  --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1

{
  echo "# Medium Chunk Paired Gate"
  echo
  echo '```text'
  echo "runs: $RUNS"
  echo "threads: $THREADS"
  echo "iters: $ITERS"
  echo "baseline: h8_bench_release"
  echo "candidate: h8_bench_release_mediumchunk"
  echo '```'
  echo
  for row in medium_interleaved_remote50 main_interleaved_remote90 \
             small_guard_local0 small_interleaved_remote90; do
    for kind in baseline chunk; do
      file="$OUT/${row}_${kind}.txt"
      echo "## ${row}_${kind}"
      echo
      echo '```text'
      grep -E 'summary|medium_geometry_id|medium_arena_id|throughput median|steady_work|peak_rss median|post_rss median|page_faults|interleaved_phase_ms' "$file" || true
      echo '```'
      echo
    done
  done
} > "$OUT/README.md"

echo "$OUT"
