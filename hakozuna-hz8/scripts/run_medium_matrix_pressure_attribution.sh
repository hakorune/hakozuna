#!/usr/bin/env bash
set -euo pipefail

HZ8_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "${HZ8_ROOT}/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

OUTDIR="${OUTDIR:-${HZ8_ROOT}/bench_results/medium_matrix_pressure_attr_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
ALLOCATORS="${ALLOCATORS:-hz8,hz3,hz4,mimalloc,tcmalloc,system}"
MATRIX_BIN="${ROOT_DIR}/bench/out/bench_matrix_malloc"

mkdir -p "${OUTDIR}" "$(dirname "${MATRIX_BIN}")"

make -C "${HZ8_ROOT}" preload >/dev/null
"${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -pthread -o "${MATRIX_BIN}" "${ROOT_DIR}/bench/bench_matrix_malloc.c"

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"
declare -A allocator_libs
for alloc in "${allocator_list[@]}"; do
  lib="$(bench_find_allocator_library "${alloc}" || true)"
  if [[ "${alloc}" != "system" && -z "${lib}" ]]; then
    echo "[ERR] missing allocator library for ${alloc}" >&2
    bench_print_allocator_hints "${alloc}"
    exit 2
  fi
  allocator_libs["${alloc}"]="${lib}"
done

csv="${OUTDIR}/samples.csv"
printf 'mode,run,throughput,steady,post_rss,peak_rss,minor_faults,work_ms,tail_ms,wall_ms,log\n' > "${csv}"

run_alloc() {
  local alloc="$1"
  local log="$2"
  shift 2
  if [[ "${alloc}" == "system" ]]; then
    "${MATRIX_BIN}" "$@" > "${log}" 2>&1
  else
    bench_run_with_allocator "${alloc}" "${allocator_libs[$alloc]}" \
      "${MATRIX_BIN}" "$@" > "${log}" 2>&1
  fi
}

run_pressure_medium_local_mix() {
  local run="$1"
  local idx=0
  for alloc in "${allocator_list[@]}"; do
    run_alloc "${alloc}" "${OUTDIR}/pressure_medium_local_run${run}_${idx}_${alloc}.log" \
      --runs 1 --threads "${THREADS}" --iters "${ITERS}" \
      --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 1 \
      --live-window "${LIVE_WINDOW}"
    idx=$((idx + 1))
  done
}

record_target() {
  local mode="$1"
  local run="$2"
  local log="${OUTDIR}/${mode}_target_run${run}.log"
  local start_ns end_ns wall_ms
  start_ns="$(date +%s%N)"
  run_alloc hz8 "${log}" \
    --runs 1 --threads "${THREADS}" --iters "${ITERS}" \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 \
    --live-window "${LIVE_WINDOW}"
  end_ns="$(date +%s%N)"
  wall_ms="$(awk -v s="${start_ns}" -v e="${end_ns}" 'BEGIN { printf "%.3f", (e - s) / 1000000.0 }')"
  awk -v mode="${mode}" -v run="${run}" -v wall="${wall_ms}" -v logfile="${log}" '
    /^throughput / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) {
        split($i, a, "="); throughput = a[2]
      }
    }
    /^steady_work / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^throughput_median=/) {
        split($i, a, "="); steady = a[2]
      }
    }
    /^post_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) {
        split($i, a, "="); post = a[2]
      }
    }
    /^peak_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) {
        split($i, a, "="); peak = a[2]
      }
    }
    /^page_faults / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^minor_median=/) {
        split($i, a, "="); faults = a[2]
      }
    }
    /^(phase_ms|interleaved_phase_ms) / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^(work_median|alloc_median)=/) {
          split($i, a, "="); work = a[2]
        } else if ($i ~ /^(tail_median|remote_median)=/) {
          split($i, a, "="); tail = a[2]
        }
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        mode, run, throughput, steady, post, peak, faults, work, tail, wall,
        logfile
    }
  ' "${log}" >> "${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  record_target none "${run}"
  run_pressure_medium_local_mix "${run}"
  record_target after_medium_local_mix "${run}"
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[row["mode"]].append(row)

def vals(items, key):
    return sorted(float(x[key]) for x in items if x.get(key))

def pct(items, key, p):
    v = vals(items, key)
    if not v:
        return 0.0
    return v[int((len(v) - 1) * p + 0.5)]

with open(dst, "w", encoding="utf-8") as f:
    f.write("# Medium Matrix Pressure Attribution\n\n")
    f.write("| Mode | median ops/s | p25 ops/s | min ops/s | steady ops/s | peak RSS | minor faults | wall ms |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|---:|\n")
    for mode in sorted(groups):
        items = groups[mode]
        f.write(
            f"| {mode} | {pct(items, 'throughput', 0.5):.3f} | "
            f"{pct(items, 'throughput', 0.25):.3f} | "
            f"{pct(items, 'throughput', 0.0):.3f} | "
            f"{pct(items, 'steady', 0.5):.3f} | "
            f"{pct(items, 'peak_rss', 0.5):.0f} | "
            f"{pct(items, 'minor_faults', 0.5):.0f} | "
            f"{pct(items, 'wall_ms', 0.5):.3f} |\n"
        )
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "${OUTDIR}"
