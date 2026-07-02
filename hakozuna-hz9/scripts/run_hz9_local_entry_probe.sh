#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_local_entry_probe}"
RUNS="${RUNS:-1}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-12000}"
mkdir -p "${OUTDIR}"

make -C "${ROOT}" hz9-standalone-check smoke-hz9localentry >/dev/null

BIN="${ROOT}/h8_hz9_local_entry_bench"
cc -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -DH8_ENABLE_DEBUG_STATS -DH8_BENCH_ATTRIBUTION \
  -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE \
  -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 \
  -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 \
  -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 \
  -DH8_MEDIUM_KEEP_REFILL_EMPTY_L1 -DH9_LOCAL_ENTRY_SPLIT_L1 \
  -I"${ROOT}/include" -I"${ROOT}/src" \
  -o "${BIN}" "${ROOT}"/src/*.c "${ROOT}"/bench/h8_bench.c \
  "${ROOT}"/bench/h8_bench_support.c "${ROOT}"/bench/h8_bench_report.c \
  "${ROOT}"/bench/h8_bench_workers.c -pthread -ldl

"${BIN}" --runs "${RUNS}" --threads "${THREADS}" --iters "${ITERS}" \
  --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0 \
  >"${OUTDIR}/medium_local0.log"

grep '^h9_local_entry ' "${OUTDIR}/medium_local0.log" \
  >"${OUTDIR}/h9_local_entry.txt"

{
  echo "# HZ9 Local Entry Probe"
  echo
  echo "\`\`\`text"
  cat "${OUTDIR}/h9_local_entry.txt"
  echo "\`\`\`"
} >"${OUTDIR}/README.md"

echo "${OUTDIR}"
