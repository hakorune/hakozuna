#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REGISTRY_SLOTS="${REGISTRY_SLOTS:-32768}"
HZ4_LIB="${HZ4_LIB:-}"
HZ4_LIB_SET_BY_USER=0
if [[ -n "${HZ4_LIB}" ]]; then
  HZ4_LIB_SET_BY_USER=1
fi

validate_slots() {
  local slots="$1"
  [[ "${slots}" =~ ^[0-9]+$ ]] || return 1
  (( slots >= 1024 )) || return 1
  (( (slots & (slots - 1)) == 0 ))
}

usage() {
  cat <<'EOF'
Usage:
  ./mac/build_mac_mid_candidate_lane_segreg.sh [--slots N]

Build the current Mac segment-registry follow-up lane:
  - hz3 observe lane is skipped
  - hz4 is rebuilt with HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1
    and HZ4_MID_FREE_BATCH_CONSUME_MIN=2
  - the default slot count is 32768; use 65536 for the slot A/B
  - route stats stay enabled so the lane can be measured right away
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --slots)
      [[ $# -ge 2 ]] || { echo "missing value for --slots" >&2; exit 1; }
      REGISTRY_SLOTS="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    --*)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
    *)
      echo "unexpected argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if ! validate_slots "${REGISTRY_SLOTS}"; then
  echo "invalid --slots value: ${REGISTRY_SLOTS} (must be power-of-two and >=1024)" >&2
  exit 1
fi

if [[ "${HZ4_LIB_SET_BY_USER}" -eq 0 ]]; then
  HZ4_LIB="${ROOT_DIR}/mac/out/observe/libhakozuna_hz4_mid_free_batch2_segreg_slots${REGISTRY_SLOTS}.so"
fi

"$ROOT_DIR/mac/check_mac_env.sh"

echo "[mac] building mid candidate lane (segment-registry follow-up)"
echo "[mac] segment-registry slots: ${REGISTRY_SLOTS}"
echo "[mac] hz4 candidate lib: ${HZ4_LIB}"
"$ROOT_DIR/mac/build_mac_observe_lane.sh" \
  --skip-hz3 \
  --hz4-lib "$HZ4_LIB" \
  --hz4-route-stats \
  --hz4-extra "-DHZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1 -DHZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS=${REGISTRY_SLOTS} -DHZ4_MID_FREE_BATCH_CONSUME_MIN=2"

echo "[mac] candidate lane ready"
echo "export HZ4_SO=${HZ4_LIB}"
echo "[mac] examples:"
printf '  HZ4_SO=%q ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 32768 50 262144\n' \
  "${HZ4_LIB}"
printf '  HZ4_SO=%q ALLOCATORS=hz4 ./mac/run_mac_mt_remote_compare.sh 16 2000000 400 16 65536 90 262144\n' \
  "${HZ4_LIB}"
