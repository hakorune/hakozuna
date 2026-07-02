#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_baseline_public_matrix}"
export RUNS="${RUNS:-10}"
export THREADS="${THREADS:-16}"
export ITERS="${ITERS:-50000}"
export RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
export ALLOCATORS="${ALLOCATORS:-hz9,system}"
export ROWS="${ROWS:-small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,guard_local0,main_local0,medium_local0}"

cat >&2 <<EOF
HZ9 standalone baseline public matrix
  OUTDIR=${OUTDIR}
  RUNS=${RUNS}
  THREADS=${THREADS}
  ITERS=${ITERS}
  ALLOCATORS=${ALLOCATORS}
  ROWS=${ROWS}

Optional external allocator overrides:
  MIMALLOC_SO=${MIMALLOC_SO:-}
  TCMALLOC_SO=${TCMALLOC_SO:-}

The hz9 allocator row uses the local copied baseline artifact from this
hakozuna-hz9 tree. No sibling HZ8 checkout is required. External
allocator rows are opt-in; set ALLOCATORS and *_SO explicitly for them.
EOF

export MATRIX_TITLE="${MATRIX_TITLE:-HZ9 Baseline Public Matrix}"
exec "${ROOT}/scripts/run_hz9_same_run_matrix_impl.sh"
