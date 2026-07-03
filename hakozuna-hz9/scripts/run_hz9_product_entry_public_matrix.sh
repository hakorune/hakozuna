#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"

export OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_product_entry_public_matrix}"
export MATRIX_TITLE="${MATRIX_TITLE:-HZ9 ProductEntry Public Matrix}"
export ALLOCATORS="${ALLOCATORS:-hz9_product,system}"
export ROWS="${ROWS:-guard_local0,small_interleaved_remote90,fixed64_local0,medium_local0,medium_interleaved_r50,main_local0,main_interleaved_r50,main_interleaved_r90}"
export MATRIX_CFLAGS="${MATRIX_CFLAGS:--DH8_MATRIX_HZ9_STATS}"
export MATRIX_LDLIBS="${MATRIX_LDLIBS:--ldl}"

cat >&2 <<EOF
[hz9-product-entry-public-matrix]
  OUTDIR=${OUTDIR}
  RUNS=${RUNS:-5}
  THREADS=${THREADS:-16}
  ITERS=${ITERS:-50000}
  ALLOCATORS=${ALLOCATORS}
  ROWS=${ROWS}
  MATRIX_CFLAGS=${MATRIX_CFLAGS}

Opt-in HZ8 frozen reference:
  HZ9_EXT_ROOT=/path/to/parent ALLOCATORS=hz8_ref,hz9_product,system
  or HZ8_REF_SO=/path/to/libhakozuna_hz8_preload.so
EOF

exec "${ROOT}/scripts/run_hz9_same_run_matrix_impl.sh"
