#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${OUT_DIR:-$ROOT/bench_results/$(date -u +%Y%m%dT%H%M%SZ)_hz9_slab_shadow_probe}"
RUNS="${RUNS:-3}"
THREADS="${THREADS:-8}"
ITERS="${ITERS:-30000}"
ROWS="${ROWS:-fixed64_local0,medium_local0,main_local0,medium_r50,main_r90}"

mkdir -p "$OUT_DIR"
make -C "$ROOT" bench-hz9mediumslabshadow >/dev/null

row_args() {
  case "$1" in
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
      ;;
    main_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1"
      ;;
    *)
      echo "unknown row: $1" >&2
      return 1
      ;;
  esac
}

IFS=',' read -r -a rows <<< "$ROWS"
for row in "${rows[@]}"; do
  args="$(row_args "$row")"
  log="$OUT_DIR/${row}.log"
  echo "[hz9-slab-shadow] row=$row args=$args"
  # shellcheck disable=SC2086
  "$ROOT/h8_bench_hz9mediumslabshadow" \
    --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" $args \
    >"$log" 2>&1
  grep -E '^(summary|throughput median|steady_work|post_rss median|peak_rss median|page_faults|interleaved_phase_ms|medium_route_shadow|medium_stats|h9_slab_shadow|h9_slab_shadow_class) ' "$log" || true
done

echo "[hz9-slab-shadow] logs=$OUT_DIR"
