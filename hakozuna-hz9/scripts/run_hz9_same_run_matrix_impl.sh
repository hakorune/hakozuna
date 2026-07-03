#!/usr/bin/env bash
set -euo pipefail

# HZ9 standalone implementation. The old run_hz8_* name remains only as a
# compatibility shim.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/bench/lib/bench_common.sh"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz9_same_run_matrix_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-50000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
ALLOCATORS="${ALLOCATORS:-hz9,hz9_legacy64k2,system}"
RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
MATRIX_BIN="${ROOT}/bench/out/bench_matrix_malloc"
MATRIX_TITLE="${MATRIX_TITLE:-HZ9 Same-Run Allocator Matrix}"

mkdir -p "${OUTDIR}" "$(dirname "${MATRIX_BIN}")"

make -C "${ROOT}" preload preload-medium64k2 preload-mediumkeeprefillempty >/dev/null
"${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -pthread -o "${MATRIX_BIN}" "${ROOT}/bench/bench_matrix_malloc.c"

local_head() {
  if [[ -e "${ROOT}/.git" ]]; then
    git -C "${ROOT}" rev-parse HEAD 2>/dev/null || printf 'unknown'
  else
    printf 'standalone-copy-no-local-git'
  fi
}

find_lib() {
  local name="$1"
  local env_name
  env_name="$(printf '%s_SO' "${name}" | tr '[:lower:]' '[:upper:]')"
  local env_value="${!env_name:-}"
  if [[ -n "${env_value}" ]]; then
    printf '%s\n' "${env_value}"
    return 0
  fi
  case "${name}" in
    hz9|hz9_product)
      printf '%s\n' "${ROOT}/libhakozuna_hz8_preload.so"
      ;;
    hz8_ref)
      printf '%s\n' "${HZ8_REF_SO:-${HZ9_EXT_ROOT}/hakozuna-hz8/libhakozuna_hz8_preload.so}"
      ;;
    hz9_legacy64k2|hz8_legacy64k2)
      printf '%s\n' "${ROOT}/libhakozuna_hz8_preload_medium64k2.so"
      ;;
    hz9_keeprefill|hz8_keeprefill)
      printf '%s\n' "${ROOT}/libhakozuna_hz8_preload_keeprefill.so"
      ;;
    hz3)
      return 1
      ;;
    hz4)
      return 1
      ;;
    mimalloc)
      return 1
      ;;
    tcmalloc)
      return 1
      ;;
    system)
      printf '\n'
      ;;
    *)
      return 1
      ;;
  esac
}

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"
declare -A allocator_lib
for alloc in "${allocator_list[@]}"; do
  lib="$(find_lib "${alloc}" || true)"
  if [[ "${alloc}" != "system" && -z "${lib}" ]]; then
    echo "[ERR] missing allocator library for ${alloc}. Set ${alloc^^}_SO." >&2
    exit 2
  fi
  allocator_lib["${alloc}"]="${lib}"
done

cat > "${OUTDIR}/README.log" <<EOF
allocator_behavior_sha=$(local_head)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
threads=${THREADS}
iters=${ITERS}
live_window=${LIVE_WINDOW}
allocators=${ALLOCATORS}
run_timeout=${RUN_TIMEOUT}
note=Standalone default compares local HZ9 artifacts and system. Set ALLOCATORS and *_SO for external allocator comparisons.
EOF
for alloc in "${allocator_list[@]}"; do
  printf 'allocator_lib[%s]=%s\n' "${alloc}" "${allocator_lib[$alloc]}" >> "${OUTDIR}/README.log"
done

csv="${OUTDIR}/samples.csv"
printf 'row,allocator,run,throughput,post_rss,peak_rss,minor_faults,work_ms,tail_ms,log\n' > "${csv}"

row_args() {
  local row="$1"
  case "${row}" in
    guard_local0)
      printf '%s\n' "--min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    medium_interleaved_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 --live-window 0"
      ;;
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    fixed24_local0)
      printf '%s\n' "--min-size 24576 --max-size 24576 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    fixed48_local0)
      printf '%s\n' "--min-size 49152 --max-size 49152 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    main_interleaved_r50)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 50 --interleaved 1 --live-window 0"
      ;;
    main_interleaved_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    medium_phase_r90)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0 --live-window ${LIVE_WINDOW}"
      ;;
    *)
      echo "[ERR] unknown row ${row}" >&2
      exit 3
      ;;
  esac
}

run_one() {
  local row="$1"
  local alloc="$2"
  local run="$3"
  local log="${OUTDIR}/${row}_run${run}_${alloc}.log"
  local args
  args="$(row_args "${row}")"
  if [[ "${alloc}" == "system" ]]; then
    # shellcheck disable=SC2086
    timeout "${RUN_TIMEOUT}" "${MATRIX_BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} > "${log}" 2>&1
  else
    # shellcheck disable=SC2086
    timeout "${RUN_TIMEOUT}" env LD_PRELOAD="${allocator_lib[$alloc]}" "${MATRIX_BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} > "${log}" 2>&1
  fi
  awk -v row="${row}" -v alloc="${alloc}" -v run="${run}" -v logfile="${log}" '
    /^throughput / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); throughput = a[2] }
      }
    }
    /^post_rss / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); post = a[2] }
      }
    }
    /^peak_rss / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); peak = a[2] }
      }
    }
    /^page_faults / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^minor_median=/) { split($i, a, "="); faults = a[2] }
      }
    }
    /^phase_ms / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^work_median=/) { split($i, a, "="); work = a[2] }
        if ($i ~ /^tail_median=/) { split($i, a, "="); tail = a[2] }
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, alloc, run, throughput, post, peak, faults, work, tail, logfile
    }
  ' "${log}" >> "${csv}"
}

if [[ -n "${ROWS:-}" ]]; then
  IFS=',' read -r -a rows <<< "${ROWS}"
else
  rows=(
    guard_local0
    small_interleaved_remote90
    fixed24_local0
    fixed48_local0
    medium_local0
    medium_interleaved_r50
    main_local0
    main_interleaved_r50
    main_interleaved_r90
  )
fi

for run in $(seq 1 "${RUNS}"); do
  count="${#allocator_list[@]}"
  for row in "${rows[@]}"; do
    for offset in $(seq 0 $((count - 1))); do
      idx=$(((offset + run - 1) % count))
      run_one "${row}" "${allocator_list[$idx]}" "${run}"
    done
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" "${MATRIX_TITLE}" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst, title = sys.argv[1], sys.argv[2], sys.argv[3]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[(row["row"], row["allocator"])].append(row)

def vals(items, key):
    return sorted(float(x[key]) for x in items if x.get(key))

def pick(items, key, p):
    v = vals(items, key)
    if not v:
        return 0.0
    return v[int((len(v)-1)*p + 0.5)]

row_names = []
for row in rows:
    if row["row"] not in row_names:
        row_names.append(row["row"])
allocs = []
for row in rows:
    if row["allocator"] not in allocs:
        allocs.append(row["allocator"])

with open(dst, "w", encoding="utf-8") as f:
    f.write(f"# {title}\n\n")
    f.write("Median ops/s. Raw samples: `samples.csv`.\n\n")
    for row in row_names:
        ranked = []
        for alloc in allocs:
            items = groups.get((row, alloc), [])
            if items:
                ranked.append((pick(items, "throughput", 0.5), alloc, items))
        ranked.sort(reverse=True)
        f.write(f"## {row}\n\n")
        f.write("| Rank | Allocator | median ops/s | p25 ops/s | min ops/s | post RSS | peak RSS | minor faults |\n")
        f.write("|---:|---|---:|---:|---:|---:|---:|---:|\n")
        for rank, (_, alloc, items) in enumerate(ranked, 1):
            f.write(
                f"| {rank} | {alloc} | {pick(items, 'throughput', 0.5):.3f} | "
                f"{pick(items, 'throughput', 0.25):.3f} | "
                f"{pick(items, 'throughput', 0.0):.3f} | "
                f"{pick(items, 'post_rss', 0.5):.0f} | "
                f"{pick(items, 'peak_rss', 0.5):.0f} | "
                f"{pick(items, 'minor_faults', 0.5):.0f} |\n"
            )
        f.write("\n")
PY

echo "${OUTDIR}"
