#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

export HZ6_WORKLOAD_PROFILE_GAP_HYBRID_LABEL="${HZ6_WORKLOAD_PROFILE_GAP_HYBRID_LABEL:-capacity_hybrid}"
export HZ6_WORKLOAD_PROFILE_GAP_HYBRID_DISPLAY_NAME="${HZ6_WORKLOAD_PROFILE_GAP_HYBRID_DISPLAY_NAME:-capacity-hybrid}"
export HZ6_WORKLOAD_PROFILE_GAP_HYBRID_BUILD_DIR="${HZ6_WORKLOAD_PROFILE_GAP_HYBRID_BUILD_DIR:-capacity_hybrid_diag}"

if [[ -z "${OUTDIR:-}" ]]; then
  export OUTDIR="${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_cliff_diag_$(date +%Y%m%d_%H%M%S)"
fi

if [[ -z "${ROWS:-}" ]]; then
  export ROWS="cliff16384"
fi

if [[ -z "${ITERS:-}" ]]; then
  export ITERS="20000"
fi

exec "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_profile_gap_diag.sh" "$@"
