#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_segment_entry_handle_probe}"
ITERS="${ITERS:-3000000}"
RUNS="${RUNS:-3}"
TOUCH="${TOUCH:-1}"
CLASSES="${CLASSES:-0 1 2 3 4 5}"
MODES="${MODES:-handlebody tokenbody tokencachebody tokencacheretire handleguardbody tlsbody tlsguardbody tlsbodychecked handlecheckedtouch tlscheckedtouch tlsepochbody tlsroute64body}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9segmententry >/dev/null

ops_from_line() {
  sed -n 's/.*ops_per_s=\([0-9.]*\).*/\1/p'
}

median_csv() {
  awk -F, '
    { v[++n] = $1 + 0 }
    END {
      if (n == 0) { print "0"; exit }
      for (i = 1; i <= n; ++i) {
        for (j = i + 1; j <= n; ++j) {
          if (v[j] < v[i]) {
            t = v[i]; v[i] = v[j]; v[j] = t
          }
        }
      }
      mid = int((n + 1) / 2)
      if (n % 2) {
        printf "%.2f", v[mid]
      } else {
        printf "%.2f", (v[mid] + v[mid + 1]) / 2.0
      }
    }'
}

{
  echo "# HZ9 SegmentEntry Handle Probe"
  echo
  echo '```text'
  echo "iters: ${ITERS}"
  echo "runs: ${RUNS}"
  echo "touch: ${TOUCH}"
  echo "classes: ${CLASSES}"
  echo "modes: ${MODES}"
  echo "purpose: compare direct handle body against TLS acquisition/check bodies"
  echo '```'
  echo
  echo "| class | mode | median ops/s | ratio_to_handlechecked | n |"
  echo "|---:|---|---:|---:|---:|"
} >"${OUTDIR}/summary.md"

for class_id in ${CLASSES}; do
  declare -A median_by_mode=()
  for mode in ${MODES}; do
    raw_file="${OUTDIR}/class_${class_id}_${mode}.txt"
    ops_file="${OUTDIR}/class_${class_id}_${mode}.ops"
    : >"${raw_file}"
    : >"${ops_file}"
    for run in $(seq 1 "${RUNS}"); do
      line="$(CLASS_ID="${class_id}" ITERS="${ITERS}" TOUCH="${TOUCH}" \
        MODE="${mode}" "${ROOT}/h8_bench_hz9segmententry")"
      printf 'run=%s %s\n' "${run}" "${line}" >>"${raw_file}"
      printf '%s\n' "${line}" | ops_from_line >>"${ops_file}"
    done
    median_by_mode["${mode}"]="$(median_csv <"${ops_file}")"
  done

  handle="${median_by_mode[handlecheckedtouch]:-0}"
  for mode in ${MODES}; do
    median="${median_by_mode[$mode]}"
    ratio="$(awk -v h="${handle}" -v v="${median}" \
      'BEGIN { if (h > 0) printf "%.3f", v / h; else printf "0.000" }')"
    ops_m="$(awk -v v="${median}" \
      'BEGIN { printf "%.2fM", v / 1000000.0 }')"
    printf '| %s | %s | %s | %s | %s |\n' \
      "${class_id}" "${mode}" "${ops_m}" "${ratio}" "${RUNS}" \
      >>"${OUTDIR}/summary.md"
  done
done

echo "[hz9-segment-entry-handle-probe] summary=${OUTDIR}/summary.md"
