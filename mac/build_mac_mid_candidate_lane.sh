#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HZ4_LIB="${HZ4_LIB:-${ROOT_DIR}/mac/out/observe/libhakozuna_hz4_mid_free_batch2_observe.so}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/build_mac_mid_candidate_lane.sh

Build the current Mac mid candidate lane:
  - hz3 observe lane stays available for comparison
  - hz4 is rebuilt with HZ4_MID_FREE_BATCH_CONSUME_MIN=2
  - route/free stats stay enabled so the lane can be measured right away
EOF
}

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  usage
  exit 0
fi

if [[ $# -gt 0 ]]; then
  echo "unknown option: $1" >&2
  usage >&2
  exit 1
fi

"$ROOT_DIR/mac/check_mac_env.sh"

echo "[mac] building mid candidate lane"
echo "[mac] hz4 candidate lib: ${HZ4_LIB}"
"$ROOT_DIR/mac/build_mac_observe_lane.sh" \
  --hz4-lib "$HZ4_LIB" \
  --hz4-route-stats \
  --hz4-extra '-DHZ4_MID_FREE_BATCH_CONSUME_MIN=2'

echo "[mac] candidate lane ready"
echo "export HZ4_SO=${HZ4_LIB}"
echo "[mac] example:"
printf '  HZ3_SO=%q HZ4_SO=%q ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4\n' \
  "${ROOT_DIR}/libhakozuna_hz3_obs.so" \
  "${HZ4_LIB}"

