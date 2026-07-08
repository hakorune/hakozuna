#!/usr/bin/env bash
# HZ11CentralPolicyCorrectness-L1: focused python_alloc/mstress correctness gate.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_central_policy_correctness_${STAMP}}"
RUNS="${RUNS:-3}"

RUN_PYTHON_ALLOC=1 \
RUN_MSTRESS=1 \
RUN_LARSON=0 \
RUN_XMALLOC=0 \
RUN_SH6BENCH=0 \
RUN_CACHE_SCRATCH=0 \
"${ROOT}/scripts/run_hz11_macro_speed_lane_gate.sh" \
  --allocators tcmalloc,hz11-thread-exit,hz11-thread-exit-cap \
  --candidate hz11-thread-exit-cap \
  --skip-span-soa-check \
  --runs "${RUNS}" \
  --outdir "${OUTDIR}"
