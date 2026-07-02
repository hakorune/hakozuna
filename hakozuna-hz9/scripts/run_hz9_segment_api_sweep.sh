#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_segment_api_sweep}"
ITERS="${ITERS:-1000000}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9segmentlocalcache-api >/dev/null

{
  echo "# HZ9 Segment API Sweep"
  echo
  echo '```text'
  echo "iters: ${ITERS}"
  echo "purpose: standalone segment substrate cost"
  echo "modes: bits=take/free_allocated, cycle_known=single known-slot cycle, active_cycle_known=active-class known-slot cycle, known_addr=slot API + address generation, slot_addr=take_slot_addr/free_allocated, fast_addr=take_addr/free_addr_fast, active_addr=active class addr API, table_addr=debug table route, table_slot_addr=debug table route returning slot, bound_addr=take_addr/free_addr"
  echo '```'
  echo
  echo "| class | mode | output |"
  echo "|---:|---|---|"
} >"${OUTDIR}/summary.md"

for class_id in 0 1 2 3 4 5; do
  for mode_id in 0 7 9 2 6 5 4 3 8 1; do
    mode="bits"
    if [[ "${mode_id}" == "1" ]]; then
      mode="bound_addr"
    elif [[ "${mode_id}" == "2" ]]; then
      mode="known_addr"
    elif [[ "${mode_id}" == "3" ]]; then
      mode="table_addr"
    elif [[ "${mode_id}" == "4" ]]; then
      mode="active_addr"
    elif [[ "${mode_id}" == "5" ]]; then
      mode="fast_addr"
    elif [[ "${mode_id}" == "6" ]]; then
      mode="slot_addr"
    elif [[ "${mode_id}" == "7" ]]; then
      mode="cycle_known"
    elif [[ "${mode_id}" == "8" ]]; then
      mode="table_slot_addr"
    elif [[ "${mode_id}" == "9" ]]; then
      mode="active_cycle_known"
    fi
    line="$(CLASS_ID="${class_id}" ITERS="${ITERS}" MODE="${mode_id}" \
      "${ROOT}/h8_bench_hz9segmentlocalcache_api")"
    printf '%s\n' "${line}" >"${OUTDIR}/class_${class_id}_${mode}.txt"
    printf '| %s | %s | `%s` |\n' "${class_id}" "${mode}" "${line}" \
      >>"${OUTDIR}/summary.md"
  done
done

echo "[hz9-segment-api-sweep] summary=${OUTDIR}/summary.md"
