#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz8_keeprefill_public_matrix}"
export RUNS="${RUNS:-10}"
export THREADS="${THREADS:-16}"
export ITERS="${ITERS:-50000}"
export RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
export ALLOCATORS="${ALLOCATORS:-hz8,mimalloc,tcmalloc,system}"
export ROWS="${ROWS:-small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,guard_local0,main_local0,medium_local0}"

cat >&2 <<EOF
HZ8 default public matrix
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
  hz8 already includes KeepRefill. Add hz8_keeprefill to ALLOCATORS only when
  you want to compare the compatibility alias explicitly.
EOF

exec "${ROOT}/scripts/run_hz8_v11_same_run_matrix.sh"
