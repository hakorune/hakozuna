#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz8_keeprefill_public_matrix}"
export RUNS="${RUNS:-10}"
export THREADS="${THREADS:-16}"
export ITERS="${ITERS:-50000}"
export RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
export ALLOCATORS="${ALLOCATORS:-hz9,system}"
export ROWS="${ROWS:-small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,guard_local0,main_local0,medium_local0}"

cat >&2 <<EOF
HZ9 standalone copied-HZ8 baseline public matrix
  OUTDIR=${OUTDIR}
  RUNS=${RUNS}
  THREADS=${THREADS}
  ITERS=${ITERS}
  ALLOCATORS=${ALLOCATORS}
  ROWS=${ROWS}

Optional external allocator overrides:
  MIMALLOC_SO=${MIMALLOC_SO:-}
  TCMALLOC_SO=${TCMALLOC_SO:-}

Compatibility note:
  This script lives in hakozuna-hz9 and uses the local copied-HZ8 baseline
  artifact. External allocators are opt-in; set ALLOCATORS and *_SO explicitly
  for a full paper/public comparison.
EOF

export MATRIX_TITLE="${MATRIX_TITLE:-HZ9 Copied-HZ8 Baseline Public Matrix}"
exec "${ROOT}/scripts/run_hz9_same_run_matrix_impl.sh"
