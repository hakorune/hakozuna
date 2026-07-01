#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUT="$ROOT/bench_results/${STAMP}_hz8_largedirect_probe_gate"

RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-50000}"
FRESH_PROCESS="${FRESH_PROCESS:-0}"
RUN_TIMEOUT="${RUN_TIMEOUT:-600s}"
MATRIX_BIN="${MATRIX_BIN:-$ROOT/bench/out/bench_matrix_malloc}"
BASELINE_SO="${BASELINE_SO:-$ROOT/libhakozuna_hz8_preload.so}"
CANDIDATE_SO="${CANDIDATE_SO:-$ROOT/libhakozuna_hz8_preload_largedirectdefault.so}"
BASELINE_NAME="${BASELINE_NAME:-baseline}"
CANDIDATE_NAME="${CANDIDATE_NAME:-largedirectdefault}"
FORCE_BUILD="${FORCE_BUILD:-1}"
ROWS="${ROWS:-cross128_r90,cross128_r0,small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,guard_local0,main_local0,medium_local0}"

mkdir -p "$OUT"

if [[ ! -x "$MATRIX_BIN" ]]; then
  echo "[ERR] missing matrix harness: $MATRIX_BIN" >&2
  exit 2
fi

if [[ "$FORCE_BUILD" == "1" ]]; then
  make -C "$ROOT" -B preload preload-largedirectdefault
elif [[ ! -f "$BASELINE_SO" || ! -f "$CANDIDATE_SO" ]]; then
  make -C "$ROOT" preload preload-largedirectdefault
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
        timeout "$RUN_TIMEOUT" env LD_PRELOAD="$BASELINE_SO" \
          "$MATRIX_BIN" --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$baseline_out"
        timeout "$RUN_TIMEOUT" env LD_PRELOAD="$CANDIDATE_SO" \
          "$MATRIX_BIN" --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$candidate_out"
      else
        timeout "$RUN_TIMEOUT" env LD_PRELOAD="$CANDIDATE_SO" \
          "$MATRIX_BIN" --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$candidate_out"
        timeout "$RUN_TIMEOUT" env LD_PRELOAD="$BASELINE_SO" \
          "$MATRIX_BIN" --runs 1 --threads "$THREADS" --iters "$ITERS" "$@" \
          >> "$baseline_out"
      fi
    done
  else
    timeout "$RUN_TIMEOUT" env LD_PRELOAD="$BASELINE_SO" \
      "$MATRIX_BIN" --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" "$@" \
      > "$baseline_out"
    timeout "$RUN_TIMEOUT" env LD_PRELOAD="$CANDIDATE_SO" \
      "$MATRIX_BIN" --runs "$RUNS" --threads "$THREADS" --iters "$ITERS" "$@" \
      > "$candidate_out"
  fi
}

run_row() {
  local row="$1"
  case "$row" in
    cross128_r90)
      run_pair "$row" \
        --min-size 16 --max-size 131072 --remote-pct 90 --interleaved 1 \
        --live-window 0
      ;;
    cross128_r0)
      run_pair "$row" \
        --min-size 16 --max-size 131072 --remote-pct 0 --interleaved 1 \
        --live-window 0
      ;;
    small_interleaved_remote90)
      run_pair "$row" \
        --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1
      ;;
    main_interleaved_r90)
      run_pair "$row" \
        --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 \
        --live-window 4096
      ;;
    medium_interleaved_r50)
      run_pair "$row" \
        --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
      ;;
    guard_local0)
      run_pair "$row" \
        --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0
      ;;
    main_local0)
      run_pair "$row" \
        --min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0
      ;;
    medium_local0)
      run_pair "$row" \
        --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0
      ;;
    *)
      echo "[ERR] unknown row: $row" >&2
      exit 3
      ;;
  esac
}

IFS=',' read -r -a row_list <<< "$ROWS"
for row in "${row_list[@]}"; do
  run_row "$row"
done

{
  echo "# HZ8 LargeDirect Default Probe Gate"
  echo
  echo '```text'
  echo "runs: $RUNS"
  echo "threads: $THREADS"
  echo "iters: $ITERS"
  echo "fresh_process: $FRESH_PROCESS"
  echo "run_timeout: $RUN_TIMEOUT"
  echo "force_build: $FORCE_BUILD"
  echo "rows: $ROWS"
  echo "matrix_bin: $MATRIX_BIN"
  echo "baseline_so: $BASELINE_SO"
  echo "candidate_so: $CANDIDATE_SO"
  echo "baseline_name: $BASELINE_NAME"
  echo "candidate_name: $CANDIDATE_NAME"
  echo '```'
  echo
  for row in "${row_list[@]}"; do
    for kind in "$BASELINE_NAME" "$CANDIDATE_NAME"; do
      file="$OUT/${row}_${kind}.txt"
      echo "## ${row}_${kind}"
      echo
      echo '```text'
      grep -E 'summary|class_map_id|medium_geometry_id|medium_arena_id|medium_residency_id|throughput median|steady_work|post_rss median|peak_rss median|page_faults|interleaved_phase_ms|direct_large_shadow|medium_stats|medium_residual_budget' "$file" || true
      echo '```'
      echo
    done
  done
} > "$OUT/README.md"

OUT="$OUT" BASELINE_NAME="$BASELINE_NAME" CANDIDATE_NAME="$CANDIDATE_NAME" ROWS="$ROWS" \
python3 - <<'PY' >> "$OUT/README.md"
import os
import re
import statistics as st
from pathlib import Path

out = Path(os.environ["OUT"])
base_name = os.environ["BASELINE_NAME"]
cand_name = os.environ["CANDIDATE_NAME"]
rows = [
    x for x in os.environ.get("ROWS", "").split(",") if x
]
run_re = re.compile(r"^run=\d+ ops/s=([0-9.]+)", re.M)
post_re = re.compile(r"post_rss median=([0-9]+)")
peak_re = re.compile(r"peak_rss median=([0-9]+)")
fault_re = re.compile(r"page_faults minor_median=([0-9]+)")

def read(path):
    text = path.read_text(errors="ignore")
    return text

def run_values(text):
    return [float(m.group(1)) for m in run_re.finditer(text)]

def one_value(regex, text):
    m = regex.search(text)
    return float(m.group(1)) if m else float("nan")

def median(xs):
    return st.median(xs) if xs else float("nan")

def p25(xs):
    if not xs:
        return float("nan")
    ys = sorted(xs)
    return ys[(len(ys) - 1) // 4]

def mib(bytes_value):
    return bytes_value / (1024.0 * 1024.0)

print("## Derived Ratios")
print()
print("| Row | Baseline median ops/s | Candidate median ops/s | Median ratio | P25 ratio | Baseline peak RSS | Candidate peak RSS | Baseline post RSS | Candidate post RSS | Minor faults median |")
print("|---|---:|---:|---:|---:|---:|---:|---:|---:|---|")
for row in rows:
    btxt = read(out / f"{row}_{base_name}.txt")
    ctxt = read(out / f"{row}_{cand_name}.txt")
    b = run_values(btxt)
    c = run_values(ctxt)
    if not b or not c:
        continue
    bpeak = one_value(peak_re, btxt)
    cpeak = one_value(peak_re, ctxt)
    bpost = one_value(post_re, btxt)
    cpost = one_value(post_re, ctxt)
    bfault = one_value(fault_re, btxt)
    cfault = one_value(fault_re, ctxt)
    print(
        f"| `{row}` | {median(b):.3f} | {median(c):.3f} | "
        f"{median(c) / median(b):.3f} | {p25(c) / p25(b):.3f} | "
        f"{mib(bpeak):.2f} MiB | {mib(cpeak):.2f} MiB | "
        f"{mib(bpost):.2f} MiB | {mib(cpost):.2f} MiB | "
        f"{bfault:.0f} -> {cfault:.0f} |"
    )
PY

echo "$OUT"
