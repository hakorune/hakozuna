#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export VARIANTS="${VARIANTS:-baseline,slabdirectuse}"
export ROWS="${ROWS:-medium_local0,main_local0,medium_r50,main_r90,guard_local0,small_interleaved_remote90}"
export RUNS="${RUNS:-3}"
export THREADS="${THREADS:-16}"
export ITERS="${ITERS:-60000}"
export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_direct_slab_use_proof}"

"${ROOT}/scripts/run_hz9_candidate_gate.sh" "${STAMP}"
