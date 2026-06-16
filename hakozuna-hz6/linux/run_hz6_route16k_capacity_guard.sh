#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
STATS_RUNS="${STATS_RUNS:-1}"
PROFILE_RUNS="${PROFILE_RUNS:-3}"
WORKLOAD_RUNS="${WORKLOAD_RUNS:-2}"
STATS_ITERS="${STATS_ITERS:-40000}"
PROFILE_ITERS="${PROFILE_ITERS:-120000}"
WORKLOAD_ITERS="${WORKLOAD_ITERS:-80000}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_route16k_capacity_guard_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

BASE_ALIAS="hz6-small-boundary-trusted-toy-map8192-external-meta-off-target"
ROUTE16K_ALIAS="hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target"
WORKLOAD_ALLOCATORS="hz6,hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target,${BASE_ALIAS},${ROUTE16K_ALIAS}"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_route16k_capacity_guard.sh [options]

Options:
  --arch ARCH         target arch (default: x86_64)
  --stats-runs N     stats/failure runs for fixed rows (default: 1)
  --profile-runs N   focused/fixed production runs (default: 3)
  --workload-runs N  workload-proxy production runs (default: 2)
  --stats-iters N    iterations for stats leg (default: 40000)
  --profile-iters N  iterations for profile leg (default: 120000)
  --workload-iters N iterations for workload leg (default: 80000)
  --outdir DIR       output directory
  --skip-builds      reuse existing HZ6/profile/bench builds
  --skip-prepare     reuse existing mimalloc/tcmalloc environment variables
  --help             show this message

This is an orchestration guard for the explicit route16K fixed-boundary RSS
profile. It does not change selected/default flags.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --stats-runs)
      STATS_RUNS="$2"
      shift 2
      ;;
    --profile-runs)
      PROFILE_RUNS="$2"
      shift 2
      ;;
    --workload-runs)
      WORKLOAD_RUNS="$2"
      shift 2
      ;;
    --stats-iters)
      STATS_ITERS="$2"
      shift 2
      ;;
    --profile-iters)
      PROFILE_ITERS="$2"
      shift 2
      ;;
    --workload-iters)
      WORKLOAD_ITERS="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-builds)
      SKIP_BUILDS=1
      shift
      ;;
    --skip-prepare)
      SKIP_PREPARE_ALLOCATORS=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

mkdir -p "$OUTDIR"

common_skip_args=()
if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  common_skip_args+=(--skip-builds)
fi

prepare_args=()
if [[ "$SKIP_PREPARE_ALLOCATORS" -eq 1 ]]; then
  prepare_args+=(--skip-prepare)
fi

{
  echo "arch=${ARCH}"
  echo "stats_runs=${STATS_RUNS}"
  echo "profile_runs=${PROFILE_RUNS}"
  echo "workload_runs=${WORKLOAD_RUNS}"
  echo "stats_iters=${STATS_ITERS}"
  echo "profile_iters=${PROFILE_ITERS}"
  echo "workload_iters=${WORKLOAD_ITERS}"
  echo "base_alias=${BASE_ALIAS}"
  echo "route16k_alias=${ROUTE16K_ALIAS}"
  echo "note=route16K fixed-boundary RSS profile guard; no selected/default change"
} > "${OUTDIR}/README.log"

stats_args=()
if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  stats_args+=(--skip-bench)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_static_table_trim_ab.sh" \
  --arch "$ARCH" \
  --runs "$STATS_RUNS" \
  --iters "$STATS_ITERS" \
  --rows fixed_mid \
  --variants external_meta_off,external_meta_off_route16384,external_meta_off_route24576 \
  --outdir "${OUTDIR}/static_stats" \
  "${stats_args[@]}" \
  --stats

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh" \
  --arch "$ARCH" \
  --runs "$PROFILE_RUNS" \
  --iters "$PROFILE_ITERS" \
  --rows focused,fixed_mid \
  --allocators "${BASE_ALIAS},${ROUTE16K_ALIAS}" \
  --outdir "${OUTDIR}/profile_frontier" \
  "${common_skip_args[@]}" \
  "${prepare_args[@]}"

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_proxy_matrix.sh" \
  --arch "$ARCH" \
  --runs "$WORKLOAD_RUNS" \
  --iters "$WORKLOAD_ITERS" \
  --rows all \
  --allocators "$WORKLOAD_ALLOCATORS" \
  --outdir "${OUTDIR}/workload_proxy" \
  "${common_skip_args[@]}" \
  "${prepare_args[@]}"

{
  echo "# HZ6 Route16K Capacity Guard"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "## Legs"
  echo
  echo "- static stats: \`${OUTDIR}/static_stats/summary.md\`"
  echo "- profile frontier: \`${OUTDIR}/profile_frontier/summary.md\`"
  echo "- workload proxy: \`${OUTDIR}/workload_proxy/summary.md\`"
  echo
  echo "## Read Rule"
  echo
  echo "Promote only as an explicit fixed-boundary profile when static stats stay"
  echo "failure-free and focused/fixed production rows keep the RSS win. Do not"
  echo "promote to selected/default while workload-proxy large live-set rows still"
  echo "require capacity-narrow or capacity-hybrid."
} | tee "${OUTDIR}/summary.md"
