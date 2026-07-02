#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_segment_route_proof}"
ITERS="${ITERS:-1000000}"
TOUCH="${TOUCH:-1}"
CLASSES="${CLASSES:-0 1 2 3 4 5}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9segmentlocalcache-local >/dev/null

run_mode() {
  local class_id="$1"
  local mode="$2"
  local active_cycle=0
  local active_route=0
  local route_interval=64

  case "${mode}" in
    direct) ;;
    active) active_cycle=1 ;;
    public) active_route=1 ;;
    exact) active_route=5 ;;
    sample8)
      active_route=6
      route_interval=8
      ;;
    sample64)
      active_route=6
      route_interval=64
      ;;
    header) active_route=7 ;;
    token) active_route=8 ;;
    pair) active_route=9 ;;
    pairfast) active_route=10 ;;
    pairdirect) active_route=11 ;;
    *)
      echo "unknown mode: ${mode}" >&2
      return 2
      ;;
  esac

  CLASS_ID="${class_id}" ITERS="${ITERS}" TOUCH="${TOUCH}" \
    ACTIVE_CYCLE="${active_cycle}" ACTIVE_ROUTE="${active_route}" \
    ROUTE_PROOF_INTERVAL="${route_interval}" \
    "${ROOT}/h8_bench_hz9segmentlocalcache_local"
}

{
  echo "# HZ9 Segment Route Proof Gate"
  echo
  echo '```text'
  echo "iters: ${ITERS}"
  echo "touch: ${TOUCH}"
  echo "classes: ${CLASSES}"
  echo "purpose: compare direct local body with public and sampled route proof"
  echo '```'
  echo
  echo "| class | mode | ops/s | ratio_to_active | raw |"
  echo "|---:|---|---:|---:|---|"
} >"${OUTDIR}/summary.md"

for class_id in ${CLASSES}; do
  declare -A raw_by_mode=()
  declare -A ops_by_mode=()
  for mode in direct active public exact sample8 sample64 header token pair pairfast pairdirect; do
    line="$(run_mode "${class_id}" "${mode}")"
    raw_by_mode["${mode}"]="${line}"
    ops="$(printf '%s\n' "${line}" |
      sed -n 's/.*ops_per_s=\([0-9.]*\).*/\1/p')"
    ops_by_mode["${mode}"]="${ops}"
    printf '%s\n' "${line}" >"${OUTDIR}/class_${class_id}_${mode}.txt"
  done
  active_ops="${ops_by_mode[active]}"
  for mode in direct active public exact sample8 sample64 header token pair pairfast pairdirect; do
    ratio="$(awk -v a="${active_ops}" -v b="${ops_by_mode[$mode]}" \
      'BEGIN { if (a > 0) printf "%.3f", b / a; else printf "0.000" }')"
    ops_m="$(awk -v v="${ops_by_mode[$mode]}" \
      'BEGIN { printf "%.2fM", v / 1000000.0 }')"
    printf '| %s | %s | %s | %s | `%s` |\n' \
      "${class_id}" "${mode}" "${ops_m}" "${ratio}" \
      "${raw_by_mode[$mode]}" >>"${OUTDIR}/summary.md"
  done
done

echo "[hz9-segment-route-proof] summary=${OUTDIR}/summary.md"
