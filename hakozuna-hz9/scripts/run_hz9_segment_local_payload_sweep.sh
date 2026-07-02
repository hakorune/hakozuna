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
  echo "modes: direct=known-slot body, active=active segment known-slot body, active_route=active direct take + route_table_slot free, active_fast=active direct take + active-first route free, active_route_probe=active-first route then known-slot free, active_range_probe=active range check then known-slot free, active_exact_probe=active exact no-fallback route then known-slot free, active_sample8/64=sampled route_table_slot proof with direct known-slot reuse, active_header_probe=slot-header route then known-slot free, active_token_probe=TLS last-token then known-slot free, active_pair_probe=active direct take then known-slot free with no route, active_pair_fast_free=active direct take then active-class direct free, active_pair_direct=single helper active take/free, route2=table route returns class+slot before free"
  echo '```'
  echo
  echo "| class | mode | output |"
  echo "|---:|---|---|"
} >"${OUTDIR}/summary.md"

for class_id in 0 1 2 3 4 5; do
  for mode in direct active active_route active_fast active_route_probe active_range_probe active_exact_probe active_sample8 active_sample64 active_header_probe active_token_probe active_pair_probe active_pair_fast_free active_pair_direct route2; do
    active_cycle=0
    active_route=0
    route_free=0
    route_interval=64
    if [[ "${mode}" == "active" ]]; then
      active_cycle=1
    elif [[ "${mode}" == "active_route" ]]; then
      active_route=1
    elif [[ "${mode}" == "active_fast" ]]; then
      active_route=2
    elif [[ "${mode}" == "active_route_probe" ]]; then
      active_route=3
    elif [[ "${mode}" == "active_range_probe" ]]; then
      active_route=4
    elif [[ "${mode}" == "active_exact_probe" ]]; then
      active_route=5
    elif [[ "${mode}" == "active_sample8" ]]; then
      active_route=6
      route_interval=8
    elif [[ "${mode}" == "active_sample64" ]]; then
      active_route=6
      route_interval=64
    elif [[ "${mode}" == "active_header_probe" ]]; then
      active_route=7
    elif [[ "${mode}" == "active_token_probe" ]]; then
      active_route=8
    elif [[ "${mode}" == "active_pair_probe" ]]; then
      active_route=9
    elif [[ "${mode}" == "active_pair_fast_free" ]]; then
      active_route=10
    elif [[ "${mode}" == "active_pair_direct" ]]; then
      active_route=11
    elif [[ "${mode}" == "route2" ]]; then
      route_free=2
    fi
    line="$(CLASS_ID="${class_id}" ITERS="${ITERS}" TOUCH="${TOUCH}" \
      ACTIVE_CYCLE="${active_cycle}" ACTIVE_ROUTE="${active_route}" \
      ROUTE_FREE="${route_free}" ROUTE_PROOF_INTERVAL="${route_interval}" \
      "${ROOT}/h8_bench_hz9segmentlocalcache_local")"
    printf '%s\n' "${line}" >"${OUTDIR}/class_${class_id}_${mode}.txt"
    printf '| %s | %s | `%s` |\n' "${class_id}" "${mode}" "${line}" \
      >>"${OUTDIR}/summary.md"
  done
done

echo "[hz9-segment-local-payload] summary=${OUTDIR}/summary.md"
