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
  echo "modes: bits=take/free_allocated, bound_addr=take_addr/free_addr"
  echo '```'
  echo
  echo "| class | mode | output |"
  echo "|---:|---|---|"
} >"${OUTDIR}/summary.md"

for class_id in 0 1 2 3 4 5; do
  for mode in bits bound_addr; do
    bound=0
    if [[ "${mode}" == "bound_addr" ]]; then
      bound=1
    fi
    line="$(CLASS_ID="${class_id}" ITERS="${ITERS}" BOUND_ADDR="${bound}" \
      "${ROOT}/h8_bench_hz9segmentlocalcache_api")"
    printf '%s\n' "${line}" >"${OUTDIR}/class_${class_id}_${mode}.txt"
    printf '| %s | %s | `%s` |\n' "${class_id}" "${mode}" "${line}" \
      >>"${OUTDIR}/summary.md"
  done
done

echo "[hz9-segment-api-sweep] summary=${OUTDIR}/summary.md"
