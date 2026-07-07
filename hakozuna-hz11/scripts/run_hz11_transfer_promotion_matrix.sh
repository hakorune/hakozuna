#!/usr/bin/env bash
# HZ11TransferPromotionMatrix-L1:
# Re-runnable promotion gate for libhz11_span_transfer.so as the HZ11 speed
# span-lane candidate. This is a gate box, not a default-lane switch.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
ROOT_DIR="${REPO_ROOT}"
source "${REPO_ROOT}/bench/lib/bench_common.sh"

RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
LIVE_WINDOW="${LIVE_WINDOW:-1024}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_transfer_promotion_$(date -u +%Y%m%dT%H%M%SZ)}"
BUILD="${BUILD:-1}"
BENCH_BIN="${BENCH_BIN:-${REPO_ROOT}/bench/out/bench_matrix_malloc}"
ALLOCATORS="${ALLOCATORS:-tcmalloc,hz11-span-soa,hz11-span-transfer}"

usage() {
  cat <<'EOF'
Usage:
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh [options]

Options:
  --allocators LIST   comma-separated allocators
  --runs N            fresh process samples per row/allocator (default 10)
  --threads N         worker threads (default 16)
  --iters N           iterations per thread (default 100000)
  --live-window N     interleaved inbox cap window (default 1024)
  --outdir DIR        output directory
  --skip-build        do not build HZ11 lanes or matrix harness
  --help              show this message

Rows:
  main_local0
  main_r50
  main_r90
  small_remote90
  medium_r50
  medium_r90
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --allocators)
      ALLOCATORS="$2"; shift 2 ;;
    --runs)
      RUNS="$2"; shift 2 ;;
    --threads)
      THREADS="$2"; shift 2 ;;
    --iters)
      ITERS="$2"; shift 2 ;;
    --live-window)
      LIVE_WINDOW="$2"; shift 2 ;;
    --outdir)
      OUTDIR="$2"; shift 2 ;;
    --skip-build)
      BUILD=0; shift ;;
    --help|-h)
      usage; exit 0 ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1 ;;
  esac
done

mkdir -p "${OUTDIR}" "$(dirname "${BENCH_BIN}")"

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" preload-span-soa preload-span-transfer >/dev/null
  "${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
    -pthread -o "${BENCH_BIN}" "${REPO_ROOT}/bench/bench_matrix_malloc.c"
fi

declare -A allocator_libs
allocator_lib() {
  local alloc="$1"
  case "${alloc}" in
    tcmalloc)
      bench_find_tcmalloc_library ;;
    hz11-span-soa)
      bench_find_first_existing "${HZ11_SPAN_SOA_SO:-}" "${ROOT}/libhz11_span_soa.so" ;;
    hz11-span-transfer)
      bench_find_first_existing "${HZ11_SPAN_TRANSFER_SO:-}" "${ROOT}/libhz11_span_transfer.so" ;;
    /*)
      bench_find_first_existing "${alloc}" ;;
    *)
      echo "unknown allocator: ${alloc}" >&2
      return 1 ;;
  esac
}

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"
for alloc in "${allocator_list[@]}"; do
  lib="$(allocator_lib "${alloc}" || true)"
  if [[ -z "${lib}" ]]; then
    echo "[ERR] missing allocator library for ${alloc}" >&2
    [[ "${alloc}" == "tcmalloc" ]] && bench_print_allocator_hints tcmalloc
    exit 2
  fi
  allocator_libs["${alloc}"]="${lib}"
done

cat > "${OUTDIR}/README.log" <<EOF
[HZ11_TRANSFER_PROMOTION] ts=$(date -u +%Y%m%dT%H%M%SZ)
[HZ11_TRANSFER_PROMOTION] root=${REPO_ROOT}
[HZ11_TRANSFER_PROMOTION] hz11_root=${ROOT}
[HZ11_TRANSFER_PROMOTION] bench_bin=${BENCH_BIN}
[HZ11_TRANSFER_PROMOTION] allocators=${ALLOCATORS}
[HZ11_TRANSFER_PROMOTION] runs=${RUNS}
[HZ11_TRANSFER_PROMOTION] threads=${THREADS}
[HZ11_TRANSFER_PROMOTION] iters=${ITERS}
[HZ11_TRANSFER_PROMOTION] live_window=${LIVE_WINDOW}
[HZ11_TRANSFER_PROMOTION] git_sha=$(git -C "${REPO_ROOT}" rev-parse --short HEAD 2>/dev/null || echo unknown)
EOF

for alloc in "${allocator_list[@]}"; do
  echo "[HZ11_TRANSFER_PROMOTION] ${alloc}=${allocator_libs[$alloc]}" | tee -a "${OUTDIR}/README.log"
done

csv="${OUTDIR}/samples.csv"
printf 'row,run,allocator,lib,throughput,throughput_p25,throughput_p75,steady,steady_p25,steady_p75,post_rss,peak_rss,minor_faults,work_ms,tail_ms,xfer_hit,xfer_miss,xfer_insert,xfer_spill,central_hit,central_miss,central_insert,refill_xfer,refill_central,refill_span,log\n' > "${csv}"

run_case() {
  local row="$1" run="$2" alloc="$3" args="$4"
  local lib="${allocator_libs[$alloc]}"
  local log="${OUTDIR}/${row}_run${run}_${alloc}.log"

  {
    echo "[CASE] row=${row}"
    echo "[CASE] run=${run}"
    echo "[CASE] alloc=${alloc}"
    echo "[CASE] lib=${lib}"
    echo "[CASE] args=${args}"
    echo
  } > "${log}"

  read -r -a argv <<< "${args}"
  if [[ "${alloc}" == "hz11-span-transfer" ]]; then
    env HZ11_DUMP_STATS=1 LD_PRELOAD="${lib}" "${BENCH_BIN}" "${argv[@]}" \
      >> "${log}" 2>&1
  else
    env LD_PRELOAD="${lib}" "${BENCH_BIN}" "${argv[@]}" >> "${log}" 2>&1
  fi

  awk -v row="${row}" -v run="${run}" -v alloc="${alloc}" \
      -v lib="${lib}" -v logfile="${log}" '
    /^throughput / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); throughput = a[2] }
        else if ($i ~ /^p25=/) { split($i, a, "="); throughput_p25 = a[2] }
        else if ($i ~ /^p75=/) { split($i, a, "="); throughput_p75 = a[2] }
      }
    }
    /^steady_work / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^throughput_median=/) { split($i, a, "="); steady = a[2] }
        else if ($i ~ /^p25=/) { split($i, a, "="); steady_p25 = a[2] }
        else if ($i ~ /^p75=/) { split($i, a, "="); steady_p75 = a[2] }
      }
    }
    /^post_rss / { for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); post = a[2] } }
    /^peak_rss / { for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); peak = a[2] } }
    /^page_faults / { for (i = 1; i <= NF; ++i) if ($i ~ /^minor_median=/) { split($i, a, "="); faults = a[2] } }
    /^(phase_ms|interleaved_phase_ms) / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^(work_median|alloc_median)=/) { split($i, a, "="); work = a[2] }
        else if ($i ~ /^(tail_median|remote_median)=/) { split($i, a, "="); tail = a[2] }
      }
    }
    /hz11_shim_exit_stats/ {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "xfer_hit") xfer_hit = a[2]
        else if (a[1] == "xfer_miss") xfer_miss = a[2]
        else if (a[1] == "xfer_insert") xfer_insert = a[2]
        else if (a[1] == "xfer_spill") xfer_spill = a[2]
        else if (a[1] == "central_hit") central_hit = a[2]
        else if (a[1] == "central_miss") central_miss = a[2]
        else if (a[1] == "central_insert") central_insert = a[2]
        else if (a[1] == "refill_xfer") refill_xfer = a[2]
        else if (a[1] == "refill_central") refill_central = a[2]
        else if (a[1] == "refill_span") refill_span = a[2]
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, run, alloc, lib, throughput, throughput_p25, throughput_p75,
        steady, steady_p25, steady_p75, post, peak, faults, work, tail,
        xfer_hit, xfer_miss, xfer_insert, xfer_spill, central_hit,
        central_miss, central_insert, refill_xfer, refill_central, refill_span,
        logfile
    }
  ' "${log}" >> "${csv}"
}

run_row() {
  local row="$1" args="$2" count="${#allocator_list[@]}"
  for run in $(seq 1 "${RUNS}"); do
    local offset=$(( (run - 1) % count ))
    for idx in $(seq 0 $((count - 1))); do
      local alloc="${allocator_list[$(((idx + offset) % count))]}"
      echo "[RUN] row=${row} run=${run} alloc=${alloc}"
      run_case "${row}" "${run}" "${alloc}" "${args}"
    done
  done
}

common="--runs 1 --threads ${THREADS} --iters ${ITERS} --touch 1 --live-window ${LIVE_WINDOW}"
run_row "main_local0" \
  "${common} --min-size 16 --max-size 32768 --remote-pct 0 --interleaved 1"
run_row "main_r50" \
  "${common} --min-size 16 --max-size 32768 --remote-pct 50 --interleaved 1"
run_row "main_r90" \
  "${common} --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1"
run_row "small_remote90" \
  "${common} --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1"
run_row "medium_r50" \
  "${common} --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
run_row "medium_r90" \
  "${common} --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 1"

python3 - "${csv}" "${OUTDIR}/summary.md" "${RUNS}" "${THREADS}" "${ITERS}" "${LIVE_WINDOW}" "${ALLOCATORS}" <<'PY'
import csv
import math
import statistics
import sys
from collections import defaultdict

src, dst, runs, threads, iters, live_window, allocator_arg = sys.argv[1:8]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[(row["row"], row["allocator"])].append(row)

def vals(items, key):
    out = []
    for item in items:
        value = item.get(key, "")
        if value != "":
            out.append(float(value))
    return out

def med(items, key):
    v = vals(items, key)
    return statistics.median(v) if v else math.nan

def pct(items, key, p):
    v = sorted(vals(items, key))
    if not v:
        return math.nan
    idx = int((len(v) - 1) * p + 0.5)
    return v[min(idx, len(v) - 1)]

def total(items, key):
    return sum(vals(items, key))

def have(row, alloc):
    return (row, alloc) in groups and groups[(row, alloc)]

def ratio(row, cand, base, key="throughput"):
    if not have(row, cand) or not have(row, base):
        return math.nan
    b = med(groups[(row, base)], key)
    return med(groups[(row, cand)], key) / b if b and not math.isnan(b) else math.nan

def mib(bytes_value):
    return bytes_value / (1024.0 * 1024.0)

target_rows = [
    "main_local0",
    "main_r50",
    "main_r90",
    "small_remote90",
    "medium_r50",
    "medium_r90",
]
allocators = [item for item in allocator_arg.split(",") if item]

checks = []
def check(name, ok, detail):
    checks.append((name, bool(ok), detail))

local_ratio = ratio("main_local0", "hz11-span-transfer", "hz11-span-soa")
check("fixed/local hit path no regression",
      not math.isnan(local_ratio) and local_ratio >= 0.97,
      f"main_local0 transfer/span-soa={local_ratio:.3f}")

for row in ("main_r50", "main_r90"):
    soa_ratio = ratio(row, "hz11-span-transfer", "hz11-span-soa")
    tc_ratio = ratio(row, "hz11-span-transfer", "tcmalloc")
    check(f"{row} improves over span-soa",
          not math.isnan(soa_ratio) and soa_ratio >= 1.10,
          f"transfer/span-soa={soa_ratio:.3f}")
    check(f"{row} tcmalloc parity",
          not math.isnan(tc_ratio) and tc_ratio >= 1.00,
          f"transfer/tcmalloc={tc_ratio:.3f}")

for row in target_rows:
    if have(row, "hz11-span-transfer") and have(row, "tcmalloc"):
        tr = med(groups[(row, "hz11-span-transfer")], "post_rss")
        tc = med(groups[(row, "tcmalloc")], "post_rss")
        check(f"{row} post RSS <= tcmalloc*1.25",
              tc > 0 and tr <= tc * 1.25,
              f"transfer={mib(tr):.2f}MiB tcmalloc*1.25={mib(tc * 1.25):.2f}MiB")

xfer_hit = total(groups.get(("main_r50", "hz11-span-transfer"), []), "xfer_hit") + \
           total(groups.get(("main_r90", "hz11-span-transfer"), []), "xfer_hit")
xfer_insert = total(groups.get(("main_r50", "hz11-span-transfer"), []), "xfer_insert") + \
              total(groups.get(("main_r90", "hz11-span-transfer"), []), "xfer_insert")
check("transfer cache is used on main remote rows",
      xfer_hit > 0 and xfer_insert > 0,
      f"xfer_hit={int(xfer_hit)} xfer_insert={int(xfer_insert)}")

for row in ("small_remote90", "medium_r50", "medium_r90"):
    soa_ratio = ratio(row, "hz11-span-transfer", "hz11-span-soa")
    tc_rss = ratio(row, "hz11-span-transfer", "tcmalloc", key="post_rss")
    soa_rss = ratio(row, "hz11-span-transfer", "hz11-span-soa", key="post_rss")
    check(f"{row} no large throughput regression vs span-soa",
          not math.isnan(soa_ratio) and soa_ratio >= 0.90,
          f"transfer/span-soa={soa_ratio:.3f}")
    check(f"{row} RSS not materially worse",
          (math.isnan(tc_rss) or tc_rss <= 1.25) and (math.isnan(soa_rss) or soa_rss <= 1.25),
          f"postRSS transfer/tcmalloc={tc_rss:.3f} transfer/span-soa={soa_rss:.3f}")

verdict = "GO" if all(ok for _, ok, _ in checks) else "NO-GO"

with open(dst, "w", encoding="utf-8") as f:
    f.write("# HZ11 Transfer Promotion Matrix L1\n\n")
    f.write(f"Conditions: RUNS={runs}, THREADS={threads}, ITERS={iters}, LIVE_WINDOW={live_window}.\n\n")
    f.write("Allocators: " + ", ".join(f"`{item}`" for item in allocators) + ".\n\n")
    f.write("## Summary\n\n")
    f.write("| Row | Allocator | median ops/s | p25 ops/s | p75 ops/s | post RSS MiB | peak RSS MiB | xfer_hit | xfer_insert | central_insert |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for row in target_rows:
        for alloc in allocators:
            items = groups.get((row, alloc), [])
            if not items:
                continue
            f.write(
                f"| {row} | {alloc} | {med(items, 'throughput'):.3f} | "
                f"{pct(items, 'throughput', 0.25):.3f} | {pct(items, 'throughput', 0.75):.3f} | "
                f"{mib(med(items, 'post_rss')):.2f} | {mib(med(items, 'peak_rss')):.2f} | "
                f"{int(total(items, 'xfer_hit'))} | {int(total(items, 'xfer_insert'))} | "
                f"{int(total(items, 'central_insert'))} |\n"
            )
    f.write("\n## Ratios\n\n")
    f.write("| Row | transfer/span-soa ops | transfer/tcmalloc ops | transfer/tcmalloc post RSS |\n")
    f.write("|---|---:|---:|---:|\n")
    for row in target_rows:
        f.write(
            f"| {row} | {ratio(row, 'hz11-span-transfer', 'hz11-span-soa'):.3f} | "
            f"{ratio(row, 'hz11-span-transfer', 'tcmalloc'):.3f} | "
            f"{ratio(row, 'hz11-span-transfer', 'tcmalloc', key='post_rss'):.3f} |\n"
        )
    f.write("\n## Gate\n\n")
    f.write(f"Verdict: **{verdict}**\n\n")
    f.write("| Check | Result | Detail |\n")
    f.write("|---|---|---|\n")
    for name, ok, detail in checks:
        f.write(f"| {name} | {'PASS' if ok else 'FAIL'} | {detail} |\n")
    f.write("\nGO means `libhz11_span_transfer.so` is eligible as the recommended HZ11 speed lane, not the default allocator.\n\n")
    f.write("Raw samples: `samples.csv`\n")
PY

echo "[DONE] HZ11 transfer promotion logs saved to ${OUTDIR}"
echo "[DONE] summary: ${OUTDIR}/summary.md"
