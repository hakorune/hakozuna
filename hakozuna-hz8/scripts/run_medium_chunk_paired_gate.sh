#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUT="$ROOT/bench_results/${STAMP}_medium_chunk_paired_gate"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
FRESH_PROCESS="${FRESH_PROCESS:-0}"
CANDIDATE_BIN="${CANDIDATE_BIN:-$ROOT/h8_bench_release_mediumchunk}"
CANDIDATE_NAME="${CANDIDATE_NAME:-chunk}"
BASELINE_BIN="${BASELINE_BIN:-$ROOT/h8_bench_release}"
BASELINE_NAME="${BASELINE_NAME:-baseline}"

mkdir -p "$OUT"

if [[ ! -x "$BASELINE_BIN" || ! -x "$CANDIDATE_BIN" ]]; then
  make -C "$ROOT" bench-release bench-release-mediumchunk
fi

run_pair() {
  local name="$1"
  shift
  local baseline_out="$OUT/${name}_${BASELINE_NAME}.txt"
  local candidate_out="$OUT/${name}_${CANDIDATE_NAME}.txt"

  if [[ "$FRESH_PROCESS" == "1" ]]; then
    : > "$baseline_out"
    : > "$candidate_out"
    for run in $(seq 1 "$RUNS"); do
      if (( run % 2 == 1 )); then
        "$BASELINE_BIN" \
          --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$baseline_out"
        "$CANDIDATE_BIN" \
          --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$candidate_out"
      else
        "$CANDIDATE_BIN" \
          --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$candidate_out"
        "$BASELINE_BIN" \
          --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$baseline_out"
      fi
    done
  else
    "$BASELINE_BIN" \
      --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" "$@" \
      > "$baseline_out"
    "$CANDIDATE_BIN" \
      --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" "$@" \
      > "$candidate_out"
  fi
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
  echo "fresh_process: $FRESH_PROCESS"
  echo "order: baseline/candidate alternates when fresh_process=1"
  echo "baseline: $BASELINE_BIN"
  echo "baseline_name: $BASELINE_NAME"
  echo "candidate: $CANDIDATE_BIN"
  echo "candidate_name: $CANDIDATE_NAME"
  echo '```'
  echo
  for row in medium_interleaved_remote50 main_interleaved_remote90 \
             small_guard_local0 small_interleaved_remote90; do
    for kind in "$BASELINE_NAME" "$CANDIDATE_NAME"; do
      file="$OUT/${row}_${kind}.txt"
      echo "## ${row}_${kind}"
      echo
      echo '```text'
      grep -E 'summary|medium_geometry_id|medium_arena_id|medium_residency_id|throughput median|steady_work|peak_rss median|post_rss median|page_faults|interleaved_phase_ms|medium_free_cache_shadow' "$file" || true
      echo '```'
      echo
    done
  done
} > "$OUT/README.md"

OUT="$OUT" BASELINE_NAME="$BASELINE_NAME" CANDIDATE_NAME="$CANDIDATE_NAME" \
python3 - <<'PY' >> "$OUT/README.md"
import os
import re
import statistics as st
from pathlib import Path

out = Path(os.environ["OUT"])
base_name = os.environ["BASELINE_NAME"]
cand_name = os.environ["CANDIDATE_NAME"]
rows = [
    "medium_interleaved_remote50",
    "main_interleaved_remote90",
    "small_guard_local0",
    "small_interleaved_remote90",
]
run_re = re.compile(r"^run=\d+ ops/s=([0-9.]+)", re.M)
fault_re = re.compile(r"page_faults minor_median=([0-9]+)")

def values(path):
    text = path.read_text(errors="ignore")
    xs = [float(m.group(1)) for m in run_re.finditer(text)]
    faults = [int(m.group(1)) for m in fault_re.finditer(text)]
    return xs, faults

def median(xs):
    return st.median(xs) if xs else float("nan")

def p25(xs):
    if not xs:
        return float("nan")
    ys = sorted(xs)
    return ys[(len(ys) - 1) // 4]

print("## Derived Ratios")
print()
print("```text")
for row in rows:
    b, bf = values(out / f"{row}_{base_name}.txt")
    c, cf = values(out / f"{row}_{cand_name}.txt")
    if not b or not c:
        continue
    print(
        f"{row}: median_ratio={median(c) / median(b):.3f} "
        f"p25_ratio={p25(c) / p25(b):.3f} "
        f"baseline_median={median(b):.3f} candidate_median={median(c):.3f}"
    )
    if bf or cf:
        print(
            f"{row}: minor_faults_median baseline={median(bf):.0f} "
            f"candidate={median(cf):.0f}"
        )
print("```")
PY

echo "$OUT"
