#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_substrate_readiness}"
RUNS="${RUNS:-5}"
ROWS="${ROWS:-guard_local0,small_interleaved_remote90,medium_local0,main_local0,medium_r50,main_r90}"
VARIANTS="${VARIANTS:-baseline,slabadaptive,slabadaptive_entry}"

mkdir -p "${OUTDIR}"

shape_stamp="${STAMP}_shape"
gate_stamp="${STAMP}_gate"

MODE=slab OUTDIR="${OUTDIR}/code_shape" \
  "${ROOT}/scripts/run_hz9_code_shape_audit.sh" "${shape_stamp}" \
  >"${OUTDIR}/code_shape.log"

RUNS="${RUNS}" ROWS="${ROWS}" VARIANTS="${VARIANTS}" \
  OUTDIR="${OUTDIR}/candidate_gate" \
  "${ROOT}/scripts/run_hz9_candidate_gate.sh" "${gate_stamp}" \
  >"${OUTDIR}/candidate_gate.log"

{
  echo "# HZ9 Substrate Readiness"
  echo
  echo '```text'
  echo "outdir: ${OUTDIR}"
  echo "runs: ${RUNS}"
  echo "rows: ${ROWS}"
  echo "variants: ${VARIANTS}"
  echo '```'
  echo
  echo "## Code Shape"
  echo
  sed -n '/| variant | text bytes |/,/Interpretation:/p' \
    "${OUTDIR}/code_shape/summary.md"
  echo
  sed -n '/## Public Entry Shape/,$p' "${OUTDIR}/code_shape/summary.md"
  echo
  echo "## Candidate Gate"
  echo
  sed -n '/| row | variant |/,$p' "${OUTDIR}/candidate_gate/summary.md"
  echo
  echo "## Read"
  echo
  echo '```text'
  echo "Promotion requires local/small no-regression, not just remote-heavy wins."
  echo "If local or small p25 regresses, keep the candidate as profile evidence."
  echo '```'
} >"${OUTDIR}/README.md"

cat "${OUTDIR}/README.md"
