#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_post_owner_page_closure}"

RUNS_SHADOW="${RUNS_SHADOW:-1}"
RUNS_ROUTE="${RUNS_ROUTE:-2}"
RUNS_MUTATION="${RUNS_MUTATION:-1}"
RUNS_READINESS="${RUNS_READINESS:-3}"
RUNS_SIDECAR="${RUNS_SIDECAR:-3}"
THREADS="${THREADS:-2}"
ITERS="${ITERS:-30000}"

ROWS="${ROWS:-guard_local0,small_interleaved_remote90,medium_local0,main_local0,medium_r50,main_r90}"
SIDECAR_VARIANTS="${SIDECAR_VARIANTS:-baseline,slabclasses_min0_sidecar2,slabclasses_min0_entry_sidecar2}"

mkdir -p "${OUTDIR}"

RUNS_SHADOW="${RUNS_SHADOW}" \
RUNS_ROUTE="${RUNS_ROUTE}" \
RUNS_MUTATION="${RUNS_MUTATION}" \
RUNS_READINESS="${RUNS_READINESS}" \
THREADS="${THREADS}" \
ITERS="${ITERS}" \
OUTDIR="${OUTDIR}/next_substrate" \
  "${ROOT}/scripts/run_hz9_next_substrate_probe.sh" \
  "${STAMP}_next_substrate" >"${OUTDIR}/next_substrate.log"

RUNS="${RUNS_SIDECAR}" \
THREADS="${THREADS}" \
ITERS="${ITERS}" \
ROWS="${ROWS}" \
VARIANTS="${SIDECAR_VARIANTS}" \
OUTDIR="${OUTDIR}/sidecar_gate" \
  "${ROOT}/scripts/run_hz9_candidate_gate.sh" \
  "${STAMP}_sidecar_gate" >"${OUTDIR}/sidecar_gate.log"

{
  echo "# HZ9 Post Owner-Page Closure Probe"
  echo
  echo '```text'
  echo "outdir: ${OUTDIR}"
  echo "threads: ${THREADS}"
  echo "iters: ${ITERS}"
  echo "runs_shadow: ${RUNS_SHADOW}"
  echo "runs_route: ${RUNS_ROUTE}"
  echo "runs_mutation: ${RUNS_MUTATION}"
  echo "runs_readiness: ${RUNS_READINESS}"
  echo "runs_sidecar: ${RUNS_SIDECAR}"
  echo "rows: ${ROWS}"
  echo "sidecar_variants: ${SIDECAR_VARIANTS}"
  echo '```'
  echo
  echo "## Next Substrate"
  echo
  sed -n '/## Class Shadow/,$p' "${OUTDIR}/next_substrate/README.md"
  echo
  echo "## Sidecar Gate"
  echo
  sed -n '/| row | variant |/,$p' "${OUTDIR}/sidecar_gate/summary.md"
  echo
  echo "## Read"
  echo
  echo '```text'
  echo "Use this as post-owner-page evidence only."
  echo "If sidecar/slab variants still regress local rows, do not keep tuning"
  echo "SlabPage entry/sidecar flags as the next default path."
  echo "The next behavior must avoid both remote admission tax and local"
  echo "owner-page/slab fixed overhead."
  echo '```'
} >"${OUTDIR}/README.md"

cat "${OUTDIR}/README.md"
