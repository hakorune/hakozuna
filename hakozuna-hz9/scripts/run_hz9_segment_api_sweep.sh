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
  echo "purpose: standalone take/free_allocated substrate cost"
  echo '```'
  echo
  echo "| class | output |"
  echo "|---:|---|"
} >"${OUTDIR}/summary.md"

for class_id in 0 1 2 3 4 5; do
  line="$(CLASS_ID="${class_id}" ITERS="${ITERS}" \
    "${ROOT}/h8_bench_hz9segmentlocalcache_api")"
  printf '%s\n' "${line}" >"${OUTDIR}/class_${class_id}.txt"
  printf '| %s | `%s` |\n' "${class_id}" "${line}" >>"${OUTDIR}/summary.md"
done

echo "[hz9-segment-api-sweep] summary=${OUTDIR}/summary.md"
