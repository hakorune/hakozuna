#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_small_transition_inventory_linux_gate_$(date -u +%Y%m%dT%H%M%SZ)}"
export CANDIDATE_TARGET="bench-release-small-transition-inventory"
export CANDIDATE="${ROOT}/h8_bench_release_small_transition_inventory"
export CANDIDATE_LABEL="small_transition_inventory"
export CANDIDATE_FLAGS="HZ8_DEFAULT_CFLAGS,H8_SMALL_TRANSITION_INVENTORY_L1"
export CANDIDATE_SMOKE_TARGET="smoke-small-transition-inventory"
export CANDIDATE_SMOKE="${ROOT}/h8_smoke_small_transition_inventory"
export CANDIDATE_SAFETY_TARGET="safety-stress-small-transition-inventory"
export CANDIDATE_SAFETY="${ROOT}/h8_safety_stress_small_transition_inventory"
export SUMMARY_TITLE="HZ8 Small Transition Inventory Linux Gate"

exec bash "${ROOT}/scripts/run_hz8_small_partial_depot_linux_gate.sh"
