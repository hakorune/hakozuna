#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HZ3_LIB="${HZ3_LIB:-${ROOT_DIR}/libhakozuna_hz3_obs.so}"
HZ4_LIB="${HZ4_LIB:-${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4_obs.so}"

BUILD_HZ3=1
BUILD_HZ4=1
HZ3_LARGE_CACHE_STATS=0
HZ3_S74_STATS=0
HZ4_OS_STATS=0
HZ4_ROUTE_STATS=0
HZ3_EXTRA=""
HZ4_EXTRA=""

usage() {
  cat <<'EOF'
Usage:
  ./mac/build_mac_observe_lane.sh [options]

Options:
  --hz3-lib PATH          hz3 observe library path (default: libhakozuna_hz3_obs.so)
  --hz4-lib PATH          hz4 observe library path (default: hakozuna-mt/libhakozuna_hz4_obs.so)
  --skip-hz3              skip hz3 observe build
  --skip-hz4              skip hz4 observe build
  --hz3-large-cache-stats enable HZ3_LARGE_CACHE_STATS=1
  --hz3-s74-stats         enable HZ3_SCALE_S74_STATS=1 for lane-batch refill stats
  --hz4-os-stats          enable HZ4_OS_STATS=1 for deeper OS-level tracing
  --hz4-route-stats       enable ST / free-route stats on hz4
  --hz3-extra FLAGS       append extra hz3 -D flags
  --hz4-extra FLAGS       append extra hz4 -D flags
  --help                  show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --hz3-lib)
      [[ $# -ge 2 ]] || { echo "missing value for --hz3-lib" >&2; exit 1; }
      HZ3_LIB="$2"
      shift 2
      ;;
    --hz4-lib)
      [[ $# -ge 2 ]] || { echo "missing value for --hz4-lib" >&2; exit 1; }
      HZ4_LIB="$2"
      shift 2
      ;;
    --skip-hz3)
      BUILD_HZ3=0
      shift
      ;;
    --skip-hz4)
      BUILD_HZ4=0
      shift
      ;;
    --hz3-large-cache-stats)
      HZ3_LARGE_CACHE_STATS=1
      shift
      ;;
    --hz3-s74-stats)
      HZ3_S74_STATS=1
      shift
      ;;
    --hz4-os-stats)
      HZ4_OS_STATS=1
      shift
      ;;
    --hz4-route-stats)
      HZ4_ROUTE_STATS=1
      shift
      ;;
    --hz3-extra)
      [[ $# -ge 2 ]] || { echo "missing value for --hz3-extra" >&2; exit 1; }
      HZ3_EXTRA="$2"
      shift 2
      ;;
    --hz4-extra)
      [[ $# -ge 2 ]] || { echo "missing value for --hz4-extra" >&2; exit 1; }
      HZ4_EXTRA="$2"
      shift 2
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

"$ROOT_DIR/mac/check_mac_env.sh"

hz3_defs=(
  -DHZ3_STATS_DUMP=1
  -DHZ3_MEDIUM_PATH_STATS=1
  -DHZ3_S203_COUNTERS=1
  -DHZ3_S204_LARSON_DIAG=1
  -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1
  -DHZ3_S196_REMOTE_DISPATCH_STATS=1
  -DHZ3_NEXT_FALLBACK_STATS=1
)
if [[ "$HZ3_LARGE_CACHE_STATS" -eq 1 ]]; then
  hz3_defs+=(-DHZ3_LARGE_CACHE_STATS=1)
fi
if [[ -n "$HZ3_EXTRA" ]]; then
  hz3_defs+=("$HZ3_EXTRA")
fi

hz4_defs=(
  -DHZ4_MID_STATS_B1=1
  -DHZ4_MID_LOCK_TIME_STATS=1
  -DHZ4_MID_STATS_B1_SC_HIST=1
  -DHZ4_MID_LOCK_TIME_STATS_SC_HIST=1
  -DHZ4_MACOS_FOREIGN_SAFE=1
)
if [[ "$HZ4_OS_STATS" -eq 1 ]]; then
  hz4_defs+=(-DHZ4_OS_STATS=1)
fi
if [[ "$HZ4_ROUTE_STATS" -eq 1 ]]; then
  hz4_defs+=(
    -DHZ4_ST_STATS_B1=1
    -DHZ4_FREE_ROUTE_STATS_B26=1
    -DHZ4_FREE_ROUTE_ORDER_GATE_STATS_B27=1
  )
fi
if [[ -n "$HZ4_EXTRA" ]]; then
  hz4_defs+=("$HZ4_EXTRA")
fi

if [[ "$BUILD_HZ3" -eq 1 ]]; then
  echo "[observe] building hz3 observe lane -> ${HZ3_LIB}"
  echo "[observe] hz3 defs: ${hz3_defs[*]}"
  gmake -C "$ROOT_DIR/hakozuna" clean all_ldpreload_scale \
    LDPRELOAD_SCALE_LIB="$HZ3_LIB" \
    HZ3_SCALE_S74_STATS="$HZ3_S74_STATS" \
    HZ3_LDPRELOAD_DEFS_EXTRA="${hz3_defs[*]}"
  echo "[observe] hz3 ready"
fi

if [[ "$BUILD_HZ4" -eq 1 ]]; then
  echo "[observe] building hz4 observe lane -> ${HZ4_LIB}"
  echo "[observe] hz4 defs: ${hz4_defs[*]}"
  gmake -C "$ROOT_DIR/hakozuna-mt" clean "$HZ4_LIB" \
    LIB_TARGET="$HZ4_LIB" \
    HZ4_DEFS_EXTRA="${hz4_defs[*]}"
  echo "[observe] hz4 ready"
fi

echo "[observe] export hints:"
if [[ "$BUILD_HZ3" -eq 1 ]]; then
  echo "export HZ3_SO=${HZ3_LIB}"
fi
if [[ "$BUILD_HZ4" -eq 1 ]]; then
  echo "export HZ4_SO=${HZ4_LIB}"
fi
echo "[observe] example:"
printf '  HZ3_SO=%q HZ4_SO=%q ./mac/run_mac_larson_compare.sh 5 4096 32768 1000 1000 0 4\n' "$HZ3_LIB" "$HZ4_LIB"
printf '  HZ3_SO=%q HZ4_SO=%q ./mac/run_mac_redis_compare.sh hz3 4 500 2000 16 256\n' "$HZ3_LIB" "$HZ4_LIB"
