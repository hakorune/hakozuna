#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

DEFAULT_PROFILES="desc10k_source1280_route40k,desc10k_source1536_route40k,desc12k_source1280_route40k,desc12k_source1536_route40k,desc12k_source1536_route48k"
DEFAULT_OUTDIR="${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_descriptor_hybrid_narrow_ladder_$(date +%Y%m%d_%H%M%S)"

export PROFILES="${PROFILES:-$DEFAULT_PROFILES}"
export DEPOT_CAPACITY="${DEPOT_CAPACITY:-2048}"
export ROWS="${ROWS:-small_proxy,cache_proxy}"
export OUTDIR="${OUTDIR:-$DEFAULT_OUTDIR}"

exec "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_descriptor_hybrid_ladder.sh" "$@"
