#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export RUNS="${RUNS:-1}"
export THREADS="${THREADS:-16}"
export ITERS="${ITERS:-60000}"
export ROWS="${ROWS:-medium_local0,main_local0,medium_r50,main_r90,guard_local0,small_interleaved_remote90}"
export VARIANTS="${VARIANTS:-baseline,slabdirectuse,localarena_dense_ownerfast_phase8,ownerpage_purelocal}"
export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_substrate_cost_matrix}"

"${ROOT}/scripts/run_hz9_candidate_gate.sh" "${STAMP}_gate" \
  >"${OUTDIR}.log"

{
  echo "# HZ9 Substrate Cost Matrix"
  echo
  echo '```text'
  echo "outdir: ${OUTDIR}"
  echo "runs: ${RUNS}"
  echo "threads: ${THREADS}"
  echo "iters: ${ITERS}"
  echo "rows: ${ROWS}"
  echo "variants: ${VARIANTS}"
  echo '```'
  echo
  sed -n '/| row | variant |/,$p' "${OUTDIR}/summary.md"
  echo
  echo "## Read"
  echo
  echo '```text'
  echo "This is direction-finding evidence only. A candidate must improve local"
  echo "medium/main rows without regressing remote/main/small gates to become an HZ9"
  echo "behavior candidate. Profile-only wins stay evidence."
  echo '```'
} >"${OUTDIR}/README.md"

cat "${OUTDIR}/README.md"
