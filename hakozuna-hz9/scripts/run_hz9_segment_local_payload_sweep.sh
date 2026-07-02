#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_segment_local_payload}"
ITERS="${ITERS:-1000000}"
TOUCH="${TOUCH:-1}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9segmentlocalcache-local >/dev/null

{
  echo "# HZ9 Segment Local Payload Sweep"
  echo
  echo '```text'
  echo "iters: ${ITERS}"
  echo "touch: ${TOUCH}"
  echo "purpose: real-payload local core versus public free boundary"
  echo "modes: direct=known-slot body, active=active segment known-slot body, active_route=active direct take + route_table_slot free, route2=table route returns class+slot before free"
  echo '```'
  echo
  echo "| class | mode | output |"
  echo "|---:|---|---|"
} >"${OUTDIR}/summary.md"

for class_id in 0 1 2 3 4 5; do
  for mode in direct active active_route route2; do
    active_cycle=0
    active_route=0
    route_free=0
    if [[ "${mode}" == "active" ]]; then
      active_cycle=1
    elif [[ "${mode}" == "active_route" ]]; then
      active_route=1
    elif [[ "${mode}" == "route2" ]]; then
      route_free=2
    fi
    line="$(CLASS_ID="${class_id}" ITERS="${ITERS}" TOUCH="${TOUCH}" \
      ACTIVE_CYCLE="${active_cycle}" ACTIVE_ROUTE="${active_route}" \
      ROUTE_FREE="${route_free}" \
      "${ROOT}/h8_bench_hz9segmentlocalcache_local")"
    printf '%s\n' "${line}" >"${OUTDIR}/class_${class_id}_${mode}.txt"
    printf '| %s | %s | `%s` |\n' "${class_id}" "${mode}" "${line}" \
      >>"${OUTDIR}/summary.md"
  done
done

echo "[hz9-segment-local-payload] summary=${OUTDIR}/summary.md"
